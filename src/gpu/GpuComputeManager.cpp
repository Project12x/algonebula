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
  {
    auto logPath = juce::File::getSpecialLocation(
      juce::File::userApplicationDataDirectory).getChildFile("Algo Nebula").getChildFile("gpu_log.txt");
    FILE *f = fopen(logPath.getFullPathName().toRawUTF8(), "a");
    if (f) {
      fprintf(f, "[GpuComputeManager] setEngine type=%d rows=%d cols=%d\n",
              static_cast<int>(type), rows, cols);
      fclose(f);
    }
  }
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

  // Adaptive timer interval for large grids
  int cells = rows * cols;
  timerIntervalMs_ = (cells > 500000) ? 48 : (cells > 100000) ? 32 : 16;

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

void GpuComputeManager::seed(uint64_t rngSeed, float density) {
  if (!simulation_)
    return;
  auto *adapter = dynamic_cast<algonebula::GridComputeAdapter *>(simulation_.get());
  if (adapter) {
    adapter->setInitSeed(rngSeed, density);
    adapter->seed();
  }
}

void GpuComputeManager::clearState() {
  if (!simulation_)
    return;
  auto *adapter = dynamic_cast<algonebula::GridComputeAdapter *>(simulation_.get());
  if (!adapter)
    return;

  // Write zeros to both state buffers
  size_t bufSize = adapter->stateBufferSize();
  std::vector<float> zeros(bufSize / sizeof(float), 0.0f);
  auto queue = wgpuDeviceGetQueue(adapter->getDevice());
  wgpuQueueWriteBuffer(queue, adapter->getStateBufferByIndex(0), 0,
                       zeros.data(), bufSize);
  wgpuQueueWriteBuffer(queue, adapter->getStateBufferByIndex(1), 0,
                       zeros.data(), bufSize);
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
  // Process mapAsync callbacks from previous frame
#ifdef WEBGPU_BACKEND_WGPU
  wgpuDevicePoll(device, false, nullptr);
#elif defined(WEBGPU_BACKEND_DAWN)
  wgpuDeviceTick(device);
#endif
  // Poll completed readbacks
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
    static int readbackCount = 0;
    int rbIdx = readbackCount++;
    for (auto &req : requests) {
      readbackMgr_.requestReadback(
          encoder, req, [br, r, c, rbIdx](const ghostsun::ReadbackResult &result) {
            br->updateFromGpu(
                reinterpret_cast<const float *>(result.data.data()), r, c);
            // Log state diagnostics on first few readbacks
            if (rbIdx < 10 || rbIdx % 60 == 0) {
              auto *data = reinterpret_cast<const float *>(result.data.data());
              int count = (int)(result.data.size() / sizeof(float));
              float sum = 0.0f, mx = 0.0f;
              int nonzero = 0;
              for (int i = 0; i < count; ++i) {
                float v = data[i];
                sum += v;
                if (v > mx) mx = v;
                if (v > 0.001f) nonzero++;
              }
              // Use getGpuLogFile from GridComputeAdapter
              FILE* f = fopen((std::string(getenv("APPDATA")) + "\\Algo Nebula\\gpu_log.txt").c_str(), "a");
              if (f) {
                fprintf(f, "[Readback#%d] cells=%d nonzero=%d sum=%.4f max=%.4f\n",
                        rbIdx, count, nonzero, sum, mx);
                fflush(f);
                fclose(f);
              }
            }
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

}