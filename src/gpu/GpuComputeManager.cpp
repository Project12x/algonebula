#include "GpuComputeManager.h"
#include "EngineAdapters.h"
#include <chrono>
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

  // Initialize readback manager
  readbackMgr_.init(device);

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

  if (!gpu.ensureInstance())
    return false;
  if (!gpu.ensureDevice(nullptr))
    return false;

  deviceReady_ = gpu.isReady();
  return deviceReady_;
}

void GpuComputeManager::shutdownGpu() {
  readbackMgr_.shutdown(); // drain pending readbacks safely
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

  auto t0 = std::chrono::steady_clock::now();

  // Poll completed readbacks from previous frame first
  readbackMgr_.poll();

  // Create command encoder
  WGPUCommandEncoderDescriptor encDesc = {};
  encDesc.label = toSV("gpu_compute_step");
  auto encoder = wgpuDeviceCreateCommandEncoder(device, &encDesc);

  // Step the simulation
  for (int i = 0; i < stepsPerFrame_; ++i) {
    simulation_->step(encoder);
  }

  // Request readback (throttled - skip if in-flight)
  if (readbackMgr_.inFlightCount() == 0) {
    auto requests = simulation_->getReadbackRequests();
    int r = rows_;
    int c = cols_;
    auto *br = &bridge_;
    for (auto &req : requests) {
      readbackMgr_.requestReadback(
          encoder, req, [br, r, c](const ghostsun::ReadbackResult &result) {
            br->updateFromGpu(
                reinterpret_cast<const float *>(result.data.data()), r, c);
          });
    }
  }

  // Submit commands
  WGPUCommandBufferDescriptor cbDesc = {};
  cbDesc.label = toSV("gpu_compute_cmdbuf");
  auto cmdBuffer = wgpuCommandEncoderFinish(encoder, &cbDesc);
  wgpuQueueSubmit(queue, 1, &cmdBuffer);
  wgpuCommandBufferRelease(cmdBuffer);
  wgpuCommandEncoderRelease(encoder);

  auto t1 = std::chrono::steady_clock::now();
  float ms = std::chrono::duration<float, std::milli>(t1 - t0).count();
  float prev = gpuStepMs_.load(std::memory_order_relaxed);
  gpuStepMs_.store(prev * 0.9f + ms * 0.1f, std::memory_order_relaxed);

  // Poll the device to process mapAsync callbacks
#ifdef WEBGPU_BACKEND_WGPU
  wgpuDevicePoll(device, false, nullptr);
#elif defined(WEBGPU_BACKEND_DAWN)
  wgpuDeviceTick(device);
#endif
}