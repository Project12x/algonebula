#include "GridComputeAdapter.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

static WGPUStringView toSV(const char *s) { return {s, s ? strlen(s) : 0}; }

// File logging for GPU debug output (stderr is invisible in standalone apps)
static FILE* getGpuLogFile() {
  static FILE* f = nullptr;
  if (!f) {
    const char* appdata = getenv("APPDATA");
    if (appdata) {
      char path[512];
      snprintf(path, sizeof(path), "%s\\Algo Nebula\\gpu_log.txt", appdata);
      f = fopen(path, "w");
    }
    if (!f) f = stderr; // fallback
  }
  return f;
}

#define GPU_LOG(fmt, ...) do { \
  FILE* _f = getGpuLogFile(); \
  fprintf(_f, fmt, ##__VA_ARGS__); \
  fflush(_f); \
} while(0)

namespace algonebula {

WGPUBuffer GridComputeAdapter::createBuffer(WGPUDevice device, uint64_t size,
                                            WGPUBufferUsage usage,
                                            const void *data) {
  WGPUBufferDescriptor desc = {};
  desc.label = toSV("grid_buffer");
  desc.size = size;
  desc.usage = usage;
  desc.mappedAtCreation = (data != nullptr);
  auto buf = wgpuDeviceCreateBuffer(device, &desc);
  if (data && buf) {
    void *mapped = wgpuBufferGetMappedRange(buf, 0, size);
    if (mapped) {
      std::memcpy(mapped, data, size);
    }
    wgpuBufferUnmap(buf);
  }
  return buf;
}

void GridComputeAdapter::uploadBuffer(WGPUQueue queue, WGPUBuffer buffer,
                                      const void *data, uint64_t size) {
  wgpuQueueWriteBuffer(queue, buffer, 0, data, size);
}

bool GridComputeAdapter::init(WGPUDevice device) {
  if (initialized_)
    return true;
  device_ = device;

  // Create shader module from source (wgpu-native v27 API)
  std::string src = getShaderSource();
  WGPUShaderSourceWGSL wgslSource = {};
  wgslSource.chain.sType = WGPUSType_ShaderSourceWGSL;
  wgslSource.code = toSV(src.c_str());

  WGPUShaderModuleDescriptor smDesc = {};
  smDesc.label = toSV("grid_compute_shader");
  smDesc.nextInChain = (WGPUChainedStruct *)&wgslSource;
  auto shaderModule = wgpuDeviceCreateShaderModule(device, &smDesc);
  if (!shaderModule) {
    GPU_LOG("[GridComputeAdapter] Failed to create shader module\n");
    return false;
  }

  bool hasDualPass = (getSecondaryEntryPoint() != nullptr);

  // Create buffers first (needed for explicit layout approach)
  auto bufSize = stateBufferSize();
  auto bufUsage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst |
                  WGPUBufferUsage_CopySrc;
  stateBuffers_[0] = createBuffer(device, bufSize, bufUsage);
  stateBuffers_[1] = createBuffer(device, bufSize, bufUsage);
  GPU_LOG("[GridComputeAdapter] stateBuffers=[%p, %p] size=%u\n", (void*)stateBuffers_[0], (void*)stateBuffers_[1], (unsigned)bufSize);

  paramsBuffer_ =
      createBuffer(device, getParamsSize(),
                   WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
  GPU_LOG("[GridComputeAdapter] paramsBuffer=%p size=%u\n", (void*)paramsBuffer_, (unsigned)getParamsSize());

  for (uint32_t i = 0; i < extraBufferCount(); ++i) {
    auto sz = extraBufferSize(i);
    auto ebuf = createBuffer(device, sz, bufUsage);
    extraBuffers_.push_back(ebuf);
  }

  // For dual-pass engines: create explicit bind group layout with ALL bindings,
  // then create an explicit pipeline layout so both pipelines share it.
  // For single-pass: use auto-layout as before.
  uint32_t totalBindings = 3 + extraBufferCount();
  WGPUPipelineLayout explicitLayout = nullptr;

  if (hasDualPass) {
    // Create explicit bind group layout entries
    std::vector<WGPUBindGroupLayoutEntry> layoutEntries(totalBindings);

    // Binding 0: uniform params
    layoutEntries[0] = {};
    layoutEntries[0].binding = 0;
    layoutEntries[0].visibility = WGPUShaderStage_Compute;
    layoutEntries[0].buffer.type = WGPUBufferBindingType_Uniform;
    layoutEntries[0].buffer.minBindingSize = getParamsSize();

    // Binding 1: stateIn (read-only storage)
    layoutEntries[1] = {};
    layoutEntries[1].binding = 1;
    layoutEntries[1].visibility = WGPUShaderStage_Compute;
    layoutEntries[1].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    layoutEntries[1].buffer.minBindingSize = bufSize;

    // Binding 2: stateOut (read-write storage)
    layoutEntries[2] = {};
    layoutEntries[2].binding = 2;
    layoutEntries[2].visibility = WGPUShaderStage_Compute;
    layoutEntries[2].buffer.type = WGPUBufferBindingType_Storage;
    layoutEntries[2].buffer.minBindingSize = bufSize;

    // Binding 3+: extra buffers (read-write storage)
    for (uint32_t i = 0; i < extraBufferCount(); ++i) {
      layoutEntries[3 + i] = {};
      layoutEntries[3 + i].binding = 3 + i;
      layoutEntries[3 + i].visibility = WGPUShaderStage_Compute;
      layoutEntries[3 + i].buffer.type = WGPUBufferBindingType_Storage;
      layoutEntries[3 + i].buffer.minBindingSize = extraBufferSize(i);
    }

    WGPUBindGroupLayoutDescriptor bglDesc = {};
    bglDesc.label = toSV("grid_explicit_bgl");
    bglDesc.entryCount = totalBindings;
    bglDesc.entries = layoutEntries.data();
    bgl_ = wgpuDeviceCreateBindGroupLayout(device, &bglDesc);
    GPU_LOG("[GridComputeAdapter] explicit bgl_=%p (bindings=%u)\n", (void*)bgl_, totalBindings);

    // Create pipeline layout with explicit BGL
    WGPUPipelineLayoutDescriptor plDesc = {};
    plDesc.label = toSV("grid_pipeline_layout");
    plDesc.bindGroupLayoutCount = 1;
    plDesc.bindGroupLayouts = &bgl_;
    explicitLayout = wgpuDeviceCreatePipelineLayout(device, &plDesc);
    GPU_LOG("[GridComputeAdapter] explicitLayout=%p\n", (void*)explicitLayout);
  }

  // Create primary compute pipeline
  WGPUComputePipelineDescriptor cpDesc = {};
  cpDesc.label = toSV("grid_compute_pipeline");
  cpDesc.layout = explicitLayout; // null for single-pass (auto), explicit for dual-pass
  cpDesc.compute.module = shaderModule;
  cpDesc.compute.entryPoint = toSV(getEntryPoint());

  pipeline_ = wgpuDeviceCreateComputePipeline(device, &cpDesc);
  if (!pipeline_) {
    GPU_LOG("[GridComputeAdapter] Failed to create compute pipeline\n");
    wgpuShaderModuleRelease(shaderModule);
    return false;
  }
  GPU_LOG("[GridComputeAdapter] pipeline_=%p\n", (void*)pipeline_);

  // Create secondary pipeline if dual-pass (uses same explicit layout)
  if (hasDualPass) {
    WGPUComputePipelineDescriptor cpDesc2 = {};
    cpDesc2.label = toSV("grid_compute_pipeline_secondary");
    cpDesc2.layout = explicitLayout; // same explicit layout
    cpDesc2.compute.module = shaderModule;
    cpDesc2.compute.entryPoint = toSV(getSecondaryEntryPoint());

    secondaryPipeline_ = wgpuDeviceCreateComputePipeline(device, &cpDesc2);
    if (!secondaryPipeline_) {
      GPU_LOG("[GridComputeAdapter] Failed to create secondary pipeline\n");
      wgpuShaderModuleRelease(shaderModule);
      return false;
    }
    GPU_LOG("[GridComputeAdapter] secondaryPipeline_=%p\n", (void*)secondaryPipeline_);
  }

  // For single-pass: get BGL from auto-layout
  if (!hasDualPass) {
    bgl_ = wgpuComputePipelineGetBindGroupLayout(pipeline_, 0);
    GPU_LOG("[GridComputeAdapter] auto bgl_=%p\n", (void*)bgl_);
  }

  if (explicitLayout)
    wgpuPipelineLayoutRelease(explicitLayout);
  wgpuShaderModuleRelease(shaderModule);

  // Create bind groups
  for (int dir = 0; dir < 2; ++dir) {
    std::vector<WGPUBindGroupEntry> entries(totalBindings);

    entries[0] = {};
    entries[0].binding = 0;
    entries[0].buffer = paramsBuffer_;
    entries[0].size = getParamsSize();

    entries[1] = {};
    entries[1].binding = 1;
    entries[1].buffer = stateBuffers_[dir];
    entries[1].size = bufSize;

    entries[2] = {};
    entries[2].binding = 2;
    entries[2].buffer = stateBuffers_[1 - dir];
    entries[2].size = bufSize;

    for (uint32_t i = 0; i < extraBufferCount(); ++i) {
      entries[3 + i] = {};
      entries[3 + i].binding = 3 + i;
      entries[3 + i].buffer = extraBuffers_[i];
      entries[3 + i].size = extraBufferSize(i);
    }

    WGPUBindGroupDescriptor bgDesc = {};
    bgDesc.label = toSV("grid_bind_group");
    bgDesc.layout = bgl_;
    bgDesc.entryCount = totalBindings;
    bgDesc.entries = entries.data();
    bindGroups_[dir] = wgpuDeviceCreateBindGroup(device, &bgDesc);
    GPU_LOG("[GridComputeAdapter] bindGroup[%d]=%p (entries=%u)\n", dir, (void*)bindGroups_[dir], totalBindings);
  }

  // Let subclass init extra buffers
  auto queue = wgpuDeviceGetQueue(device);
  initExtraBuffers(device, queue);

  initialized_ = true;
  GPU_LOG("[GridComputeAdapter] init complete: rows=%d cols=%d dualPass=%d\n", rows_, cols_, hasDualPass);
  seed();
  return true;
}

void GridComputeAdapter::shutdown() {
  if (!initialized_)
    return;

  for (auto &bg : bindGroups_) {
    if (bg)
      wgpuBindGroupRelease(bg);
    bg = nullptr;
  }
  for (auto &bg : secondaryBindGroups_) {
    if (bg)
      wgpuBindGroupRelease(bg);
    bg = nullptr;
  }
  for (auto &sb : stateBuffers_) {
    if (sb)
      wgpuBufferRelease(sb);
    sb = nullptr;
  }
  for (auto &eb : extraBuffers_) {
    if (eb)
      wgpuBufferRelease(eb);
  }
  extraBuffers_.clear();
  if (paramsBuffer_) {
    wgpuBufferRelease(paramsBuffer_);
    paramsBuffer_ = nullptr;
  }
  if (pipeline_) {
    wgpuComputePipelineRelease(pipeline_);
    pipeline_ = nullptr;
  }
  if (secondaryPipeline_) {
    wgpuComputePipelineRelease(secondaryPipeline_);
    secondaryPipeline_ = nullptr;
  }
  if (bgl_) {
    wgpuBindGroupLayoutRelease(bgl_);
    bgl_ = nullptr;
  }
  if (secondaryBgl_) {
    wgpuBindGroupLayoutRelease(secondaryBgl_);
    secondaryBgl_ = nullptr;
  }
  initialized_ = false;
}

void GridComputeAdapter::step(WGPUCommandEncoder encoder) {
  if (!initialized_)
    return;

  // Log first few frames for diagnostics
  if (frame_ < 3) {
    GPU_LOG("[GridComputeAdapter] step frame=%u current=%d secondary=%p\n",
            frame_, current_, (void*)secondaryPipeline_);
  }

  // Update params
  std::vector<uint8_t> paramData(getParamsSize());
  writeParams(paramData.data());

  if (frame_ < 1) {
    GPU_LOG("[GridComputeAdapter] params (hex, %u bytes): ", (unsigned)getParamsSize());
    // Log as uint32_t values for easy reading
    auto* p32 = reinterpret_cast<const uint32_t*>(paramData.data());
    for (uint32_t i = 0; i < getParamsSize() / 4; ++i) {
      GPU_LOG("%08X ", p32[i]);
    }
    GPU_LOG("\n");
    // Also log as floats for the float params
    auto* pf = reinterpret_cast<const float*>(paramData.data());
    GPU_LOG("[GridComputeAdapter] params (float): ");
    for (uint32_t i = 0; i < getParamsSize() / 4; ++i) {
      GPU_LOG("%.6f ", pf[i]);
    }
    GPU_LOG("\n");
  }
  auto queue = wgpuDeviceGetQueue(device_);
  wgpuQueueWriteBuffer(queue, paramsBuffer_, 0, paramData.data(),
                       paramData.size());

  // Primary dispatch (grid pass)
  WGPUComputePassDescriptor passDesc = {};
  passDesc.label = toSV("grid_step");
  auto pass = wgpuCommandEncoderBeginComputePass(encoder, &passDesc);
  wgpuComputePassEncoderSetPipeline(pass, pipeline_);
  wgpuComputePassEncoderSetBindGroup(pass, 0, bindGroups_[current_], 0,
                                     nullptr);

  uint32_t wgX = (cols_ + 15) / 16;
  uint32_t wgY = (rows_ + 15) / 16;
  wgpuComputePassEncoderDispatchWorkgroups(pass, wgX, wgY, 1);
  wgpuComputePassEncoderEnd(pass);
  wgpuComputePassEncoderRelease(pass);

  if (frame_ < 3 && secondaryPipeline_) {
    GPU_LOG("[GridComputeAdapter] primary dispatch done, starting secondary\n");
  }

  // Secondary dispatch (particle/walker pass) if dual-pass engine
  if (secondaryPipeline_) {
    WGPUComputePassDescriptor passDesc2 = {};
    passDesc2.label = toSV("grid_step_secondary");
    auto pass2 = wgpuCommandEncoderBeginComputePass(encoder, &passDesc2);
    wgpuComputePassEncoderSetPipeline(pass2, secondaryPipeline_);
    wgpuComputePassEncoderSetBindGroup(pass2, 0, bindGroups_[current_],
                                       0, nullptr);

    uint32_t sx, sy, sz;
    getSecondaryDispatch(sx, sy, sz);
    if (frame_ < 3) {
      GPU_LOG("[GridComputeAdapter] secondary dispatch: %u x %u x %u\n", sx, sy, sz);
    }
    wgpuComputePassEncoderDispatchWorkgroups(pass2, sx, sy, sz);
    wgpuComputePassEncoderEnd(pass2);
    wgpuComputePassEncoderRelease(pass2);
  }

  // Flip ping-pong
  current_ = 1 - current_;
  ++frame_;
}

std::vector<ghostsun::ReadbackRequest>
GridComputeAdapter::getReadbackRequests() {
  if (!initialized_ || (frame_ % readbackFrameSkip_ != 0))
    return {};

  ghostsun::ReadbackRequest req;
  req.srcBuffer = stateBuffers_[current_]; // read from latest output
  req.offset = 0;
  req.size = stateBufferSize();
  req.id = 0;
  return {req};
}

void GridComputeAdapter::seed() {
  if (!initialized_)
    return;

  std::vector<float> state(cellCount() * floatsPerCell(), 0.0f);
  generateInitialState(state);

  auto queue = wgpuDeviceGetQueue(device_);
  wgpuQueueWriteBuffer(queue, stateBuffers_[0], 0, state.data(),
                       state.size() * sizeof(float));
  wgpuQueueWriteBuffer(queue, stateBuffers_[1], 0, state.data(),
                       state.size() * sizeof(float));
  current_ = 0;
  frame_ = 0;
}

void GridComputeAdapter::generateInitialState(std::vector<float> &state) const {
  uint64_t rng = rngSeed_;
  int densityPct = static_cast<int>(density_ * 100.0f);
  if (densityPct < 1) densityPct = 1;
  if (densityPct > 100) densityPct = 100;
  for (auto &v : state) {
    rng = rng * 6364136223846793005ULL + 1442695040888963407ULL;
    v = ((rng >> 33) % 100 < static_cast<uint64_t>(densityPct)) ? 1.0f : 0.0f;
  }
}

} // namespace algonebula