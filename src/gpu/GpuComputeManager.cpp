#include "GpuComputeManager.h"
#include "EngineAdapters.h"
#include <cstdio>
#include <cstring>
#include <ghostsun_render/GpuDevice.h>

// Backend-specific extensions for device polling
#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h> // wgpuDevicePoll
#endif

bool GpuComputeManager::setEngine(EngineType type, int rows, int cols) {
  stop();
  rows_ = rows;
  cols_ = cols;
  currentType_ = type;

  // Create the appropriate adapter
  switch (type) {
  case EngineType::GoL:
    simulation_ = std::make_unique<algonebula::GoLCompute>(rows, cols);
    break;
  case EngineType::BriansBrain:
    simulation_ = std::make_unique<algonebula::BriansBrainCompute>(rows, cols);
    break;
  case EngineType::CyclicCA:
    simulation_ = std::make_unique<algonebula::CyclicCACompute>(rows, cols);
    break;
  case EngineType::ReactionDiffusion:
    simulation_ =
        std::make_unique<algonebula::ReactionDiffusionCompute>(rows, cols);
    break;
  case EngineType::Lenia:
    simulation_ = std::make_unique<algonebula::Lenia2DCompute>(rows, cols);
    break;
  case EngineType::ParticleSwarm:
    simulation_ =
        std::make_unique<algonebula::ParticleSwarmCompute>(rows, cols);
    break;
  case EngineType::BrownianField:
    simulation_ =
        std::make_unique<algonebula::BrownianFieldCompute>(rows, cols);
    break;
  default:
    return false;
  }

  bridge_.resize(rows, cols);

  if (!ensureDevice()) {
    fprintf(stderr, "[GpuComputeManager] Failed to initialize GPU device\n");
    return false;
  }

  auto device = ghostsun::GpuDevice::getInstance().getDevice();
  if (!simulation_->init(device)) {
    fprintf(stderr,
            "[GpuComputeManager] Shader/pipeline init failed for engine %d\n",
            static_cast<int>(type));
    return false;
  }
  return true;
}

bool GpuComputeManager::ensureDevice() {
  auto &gpu = ghostsun::GpuDevice::getInstance();
  if (gpu.isReady()) {
    deviceReady_ = true;
    return true;
  }

  // For headless compute (no surface), pass nullptr.
  if (!gpu.ensureInstance())
    return false;
  if (!gpu.ensureDevice(nullptr))
    return false;

  deviceReady_ = gpu.isReady();
  return deviceReady_;
}

void GpuComputeManager::shutdownGpu() {
  if (simulation_)
    simulation_->shutdown();
  simulation_.reset();
  deviceReady_ = false;
}

void GpuComputeManager::seed(uint64_t /*rngSeed*/, float /*density*/) {
  if (simulation_)
    simulation_->seed();
}

void GpuComputeManager::clearState() {
  if (simulation_)
    simulation_->reset();
}

// Context for async readback
struct ReadbackContext {
  WGPUBuffer staging;
  uint64_t size;
  int rows, cols;
  GpuGridBridge *bridge;
  ghostsun::ComputeSimulation *sim;
  uint32_t reqId;
};

static void onMapCallback(WGPUMapAsyncStatus status, WGPUStringView /*message*/,
                          void *userdata1, void * /*userdata2*/) {
  auto *ctx = static_cast<ReadbackContext *>(userdata1);
  if (status == WGPUMapAsyncStatus_Success) {
    const void *mapped =
        wgpuBufferGetConstMappedRange(ctx->staging, 0, ctx->size);
    if (mapped) {
      // Feed to bridge for audio thread
      ctx->bridge->updateFromGpu(static_cast<const float *>(mapped), ctx->rows,
                                 ctx->cols);

      // Also notify the simulation
      ghostsun::ReadbackResult result;
      result.data.resize(ctx->size);
      std::memcpy(result.data.data(), mapped, ctx->size);
      result.id = ctx->reqId;
      ctx->sim->onReadbackComplete(result);
    }
  }
  wgpuBufferUnmap(ctx->staging);
  wgpuBufferRelease(ctx->staging);
  delete ctx;
}

static WGPUStringView toSV(const char *s) { return {s, s ? strlen(s) : 0}; }

void GpuComputeManager::timerCallback() {
  if (!running_ || !simulation_ || !deviceReady_)
    return;

  auto &gpu = ghostsun::GpuDevice::getInstance();
  if (gpu.isDeviceLost()) {
    stop();
    return;
  }

  auto device = gpu.getDevice();
  auto queue = gpu.getQueue();

  // Create command encoder
  WGPUCommandEncoderDescriptor encDesc = {};
  encDesc.label = toSV("gpu_compute_step");
  auto encoder = wgpuDeviceCreateCommandEncoder(device, &encDesc);

  // Step the simulation
  for (int i = 0; i < stepsPerFrame_; ++i) {
    simulation_->step(encoder);
  }

  // Submit commands
  WGPUCommandBufferDescriptor cbDesc = {};
  cbDesc.label = toSV("gpu_compute_cmdbuf");
  auto cmdBuffer = wgpuCommandEncoderFinish(encoder, &cbDesc);
  wgpuQueueSubmit(queue, 1, &cmdBuffer);
  wgpuCommandBufferRelease(cmdBuffer);
  wgpuCommandEncoderRelease(encoder);

  // Handle readback
  auto requests = simulation_->getReadbackRequests();
  for (auto &req : requests) {
    // Create staging buffer
    WGPUBufferDescriptor stagingDesc = {};
    stagingDesc.label = toSV("readback_staging");
    stagingDesc.size = req.size;
    stagingDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    auto staging = wgpuDeviceCreateBuffer(device, &stagingDesc);

    // Copy from GPU buffer to staging
    WGPUCommandEncoderDescriptor enc2Desc = {};
    enc2Desc.label = toSV("readback_copy");
    auto enc2 = wgpuDeviceCreateCommandEncoder(device, &enc2Desc);
    wgpuCommandEncoderCopyBufferToBuffer(enc2, req.srcBuffer, req.offset,
                                         staging, 0, req.size);
    WGPUCommandBufferDescriptor cb2Desc = {};
    cb2Desc.label = toSV("readback_cmdbuf");
    auto cmd2 = wgpuCommandEncoderFinish(enc2, &cb2Desc);
    wgpuQueueSubmit(queue, 1, &cmd2);
    wgpuCommandBufferRelease(cmd2);
    wgpuCommandEncoderRelease(enc2);

    // Map the staging buffer asynchronously (wgpu-native v27 API)
    auto *ctx = new ReadbackContext{staging,  req.size,          rows_, cols_,
                                    &bridge_, simulation_.get(), req.id};

    WGPUBufferMapCallbackInfo cbInfo = {};
    cbInfo.mode = WGPUCallbackMode_AllowProcessEvents;
    cbInfo.callback = onMapCallback;
    cbInfo.userdata1 = ctx;
    cbInfo.userdata2 = nullptr;

    wgpuBufferMapAsync(staging, WGPUMapMode_Read, 0, req.size, cbInfo);
  }

  // Poll the device to process mapAsync callbacks
#ifdef WEBGPU_BACKEND_WGPU
  wgpuDevicePoll(device, false, nullptr);
#elif defined(WEBGPU_BACKEND_DAWN)
  wgpuDeviceTick(device);
#endif
}
