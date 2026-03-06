#include "GridComputeAdapter.h"
#include <cstdio>
#include <cstring>


static WGPUStringView toSV(const char *s) { return {s, s ? strlen(s) : 0}; }

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
    fprintf(stderr, "[GridComputeAdapter] Failed to create shader module\n");
    return false;
  }

  // Create compute pipeline with auto layout
  WGPUComputePipelineDescriptor cpDesc = {};
  cpDesc.label = toSV("grid_compute_pipeline");
  cpDesc.layout = nullptr; // auto layout
  cpDesc.compute.module = shaderModule;
  cpDesc.compute.entryPoint = toSV(getEntryPoint());

  pipeline_ = wgpuDeviceCreateComputePipeline(device, &cpDesc);
  if (!pipeline_) {
    fprintf(stderr, "[GridComputeAdapter] Failed to create compute pipeline\n");
    wgpuShaderModuleRelease(shaderModule);
    return false;
  }

  // Get bind group layout from auto-layout pipeline
  bgl_ = wgpuComputePipelineGetBindGroupLayout(pipeline_, 0);
  fprintf(stderr, "[GridComputeAdapter] bgl_=%p\n", (void*)bgl_);
  wgpuShaderModuleRelease(shaderModule);

  // Create buffers
  auto bufSize = stateBufferSize();
  auto bufUsage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst |
                  WGPUBufferUsage_CopySrc;
  stateBuffers_[0] = createBuffer(device, bufSize, bufUsage);
  stateBuffers_[1] = createBuffer(device, bufSize, bufUsage);
  fprintf(stderr, "[GridComputeAdapter] stateBuffers=[%p, %p] size=%u\n", (void*)stateBuffers_[0], (void*)stateBuffers_[1], (unsigned)bufSize);

  // Params buffer
  paramsBuffer_ =
      createBuffer(device, getParamsSize(),
                   WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
  fprintf(stderr, "[GridComputeAdapter] paramsBuffer=%p size=%u\n", (void*)paramsBuffer_, (unsigned)getParamsSize());

  // Extra buffers
  for (uint32_t i = 0; i < extraBufferCount(); ++i) {
    auto sz = extraBufferSize(i);
    auto ebuf = createBuffer(device, sz, bufUsage);
    extraBuffers_.push_back(ebuf);
  }

  // Create bind groups (two: one for each ping-pong direction)
  for (int dir = 0; dir < 2; ++dir) {
    uint32_t totalBindings = 3 + extraBufferCount();
    std::vector<WGPUBindGroupEntry> entries(totalBindings);

    entries[0] = {};
    entries[0].binding = 0;
    entries[0].buffer = paramsBuffer_;
    entries[0].size = getParamsSize();

    entries[1] = {};
    entries[1].binding = 1;
    entries[1].buffer = stateBuffers_[dir]; // read from current
    entries[1].size = bufSize;

    entries[2] = {};
    entries[2].binding = 2;
    entries[2].buffer = stateBuffers_[1 - dir]; // write to other
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
    fprintf(stderr, "[GridComputeAdapter] bindGroup[%d]=%p (entries=%u)\n", dir, (void*)bindGroups_[dir], totalBindings);
  }

  // Let subclass init extra buffers
  auto queue = wgpuDeviceGetQueue(device);
  initExtraBuffers(device, queue);

  initialized_ = true;
  fprintf(stderr, "[GridComputeAdapter] init complete: rows=%d cols=%d\n", rows_, cols_);
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
  if (bgl_) {
    wgpuBindGroupLayoutRelease(bgl_);
    bgl_ = nullptr;
  }
  initialized_ = false;
}

void GridComputeAdapter::step(WGPUCommandEncoder encoder) {
  if (!initialized_)
    return;

  // Update params
  std::vector<uint8_t> paramData(getParamsSize());
  writeParams(paramData.data());
  auto queue = wgpuDeviceGetQueue(device_);
  wgpuQueueWriteBuffer(queue, paramsBuffer_, 0, paramData.data(),
                       paramData.size());

  // Dispatch compute
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
  // Default: 25% random density
  uint64_t rng = 42;
  for (auto &v : state) {
    rng = rng * 6364136223846793005ULL + 1442695040888963407ULL;
    v = ((rng >> 33) % 100 < 25) ? 1.0f : 0.0f;
  }
}

} // namespace algonebula