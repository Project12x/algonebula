#include "GpuComputeManager.h"
#include "EngineAdapters.h"
#include "Lenia3DCompute.h"
#include <chrono>
#include <cmath>
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
  case EngineType::Lenia3D: {
    int N = 64; // 64^3 grid
    simulation_ = std::make_unique<algonebula::Lenia3DCompute>(N);
    rows_ = N;
    cols_ = N;
    break;
  }
  default:
    return false;
  }

  // For 3D, the bridge is the 2D MIP projection (NxN)
  bridge_.resize(rows_, cols_);

  // 3D always at 60fps; 2D adaptive
  if (type == EngineType::Lenia3D) {
    timerIntervalMs_ = 16;
  } else {
    int cells = rows_ * cols_;
    timerIntervalMs_ = (cells > 500000) ? 48 : (cells > 100000) ? 32 : 16;
  }

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

  // Initialize VolumeRenderer for 3D engines
  if (type == EngineType::Lenia3D) {
    volumeRendererReady_ = volumeRenderer_.init(
        device, WGPUTextureFormat_BGRA8Unorm);
    if (volumeRendererReady_) {
      // Pre-create volume texture at grid size
      auto *lenia3d = dynamic_cast<algonebula::Lenia3DCompute *>(
          simulation_.get());
      int N = lenia3d ? lenia3d->getGridSize() : 64;
      // Upload a dummy frame to create the volume texture
      std::vector<float> zeros(N * N * N, 0.0f);
      volumeRenderer_.setVolumeData(zeros.data(), N, N, N);
      // Init pixel readback manager
      pixelReadbackMgr_.init(device);
    }
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
  readbackMgr_.shutdown();
  pixelReadbackMgr_.shutdown();
  volumeRenderer_.shutdown();
  volumeRendererReady_ = false;
  if (offscreenView_) { wgpuTextureViewRelease(offscreenView_); offscreenView_ = nullptr; }
  if (offscreenTexture_) { wgpuTextureDestroy(offscreenTexture_); wgpuTextureRelease(offscreenTexture_); offscreenTexture_ = nullptr; }
  offscreenW_ = offscreenH_ = 0;
  if (simulation_)
    simulation_->shutdown();
  simulation_.reset();
  deviceReady_ = false;
}

void GpuComputeManager::seed(uint64_t rngSeed, float density) {
  if (!simulation_)
    return;
  // 3D Lenia has its own seed method
  auto *lenia3d = dynamic_cast<algonebula::Lenia3DCompute *>(simulation_.get());
  if (lenia3d) {
    auto queue = wgpuDeviceGetQueue(
        ghostsun::GpuDevice::getInstance().getDevice());
    lenia3d->seed(queue, rngSeed, density);
    return;
  }
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
#ifdef WEBGPU_BACKEND_WGPU
  wgpuDevicePoll(device, false, nullptr);
#elif defined(WEBGPU_BACKEND_DAWN)
  wgpuDeviceTick(device);
#endif
  readbackMgr_.poll();
  if (is3D()) pixelReadbackMgr_.poll();

  WGPUCommandEncoderDescriptor encDesc = {};
  encDesc.label = toSV("gpu_compute_step");
  auto encoder = wgpuDeviceCreateCommandEncoder(device, &encDesc);

  // Step the simulation
  for (int i = 0; i < stepsPerFrame_; ++i) {
    simulation_->step(encoder);
  }

  // 3D: blit state buffer to volume texture + render
  if (is3D() && volumeRendererReady_) {
    auto *lenia3d = dynamic_cast<algonebula::Lenia3DCompute *>(
        simulation_.get());
    if (lenia3d && lenia3d->getStateBuffer()) {
      volumeRenderer_.blitBufferToVolume(encoder,
                                          lenia3d->getStateBuffer());
      renderVolumeFrame(device, encoder);
    }
  }

  // Request grid readback (throttled)
  if (readbackMgr_.inFlightCount() == 0) {
    auto requests = simulation_->getReadbackRequests();
    int r = rows_;
    int c = cols_;
    int N3 = is3D() ? rows_ : 0; // N for 3D MIP projection
    auto *br = &bridge_;
    static int readbackCount = 0;
    int rbIdx = readbackCount++;
    for (auto &req : requests) {
      readbackMgr_.requestReadback(
          encoder, req, [br, r, c, N3, rbIdx](const ghostsun::ReadbackResult &result) {
            const float *data =
                reinterpret_cast<const float *>(result.data.data());

            if (N3 > 0) {
              // 3D MIP projection: max intensity along Z
              int N = N3;
              std::vector<float> mip(N * N, 0.0f);
              for (int z = 0; z < N; ++z) {
                for (int y = 0; y < N; ++y) {
                  for (int x = 0; x < N; ++x) {
                    int idx3 = z * N * N + y * N + x;
                    int idx2 = y * N + x;
                    if (data[idx3] > mip[idx2])
                      mip[idx2] = data[idx3];
                  }
                }
              }
              br->updateFromGpu(mip.data(), N, N);
            } else {
              br->updateFromGpu(data, r, c);
            }

            // Diagnostics
            if (rbIdx < 10 || rbIdx % 60 == 0) {
              int count = (int)(result.data.size() / sizeof(float));
              float sum = 0.0f, mx = 0.0f;
              int nonzero = 0;
              for (int i = 0; i < count; ++i) {
                float v = data[i];
                sum += v;
                if (v > mx) mx = v;
                if (v > 0.001f) nonzero++;
              }
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

  // Auto-orbit for 3D
  if (is3D()) {
    time_ += 0.016f;
    // Only auto-orbit if user isn't dragging (orbit stays wherever user left it)
  }
}

void GpuComputeManager::renderVolumeFrame(WGPUDevice device,
                                           WGPUCommandEncoder encoder) {
  // Determine render size (component-sized, not fullscreen)
  int w = offscreenW_ > 0 ? offscreenW_ : 400;
  int h = offscreenH_ > 0 ? offscreenH_ : 400;

  if (!offscreenTexture_ || offscreenW_ != w || offscreenH_ != h) {
    createOffscreenTexture(device, w, h);
  }
  if (!offscreenView_) return;

  volumeRenderer_.setRenderSize(w, h);

  ghostsun::VolumeRenderParams vp;
  vp.cameraOrbit = cameraOrbit_;
  vp.cameraElevation = cameraElevation_;
  vp.cameraDistance = cameraDistance_;
  vp.densityThreshold = 0.05f;
  vp.brightness = 1.2f;
  vp.colorMode = colorMode_;
  vp.time = time_;
  vp.audioLevel = audioLevel_;

  volumeRenderer_.render(encoder, offscreenView_, vp);

  // Readback pixels to JUCE Image
  if (pixelReadbackMgr_.inFlightCount() == 0) {
    ghostsun::ReadbackRequest pixReq;
    // We need to copy the rendered texture to a buffer first.
    // Create a staging buffer for the pixel readback.
    size_t rowPitch = ((w * 4 + 255) & ~255); // 256-byte aligned
    size_t bufSize = rowPitch * h;

    WGPUBufferDescriptor stagingDesc = {};
    stagingDesc.label = toSV("vol_pixel_staging");
    stagingDesc.size = bufSize;
    stagingDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    auto staging = wgpuDeviceCreateBuffer(device, &stagingDesc);

    WGPUTexelCopyTextureInfo src = {};
    src.texture = offscreenTexture_;
    WGPUTexelCopyBufferInfo dst = {};
    dst.buffer = staging;
    dst.layout.bytesPerRow = (uint32_t)rowPitch;
    dst.layout.rowsPerImage = (uint32_t)h;
    WGPUExtent3D extent = {(uint32_t)w, (uint32_t)h, 1};
    wgpuCommandEncoderCopyTextureToBuffer(encoder, &src, &dst, &extent);

    pixReq.srcBuffer = staging;
    pixReq.offset = 0;
    pixReq.size = bufSize;

    int cw = w, ch = h;
    size_t cRowPitch = rowPitch;
    auto *self = this;
    pixelReadbackMgr_.requestReadback(
        encoder, pixReq,
        [self, cw, ch, cRowPitch, staging](const ghostsun::ReadbackResult &result) {
          juce::Image img(juce::Image::ARGB, cw, ch, false);
          juce::Image::BitmapData bmp(img, juce::Image::BitmapData::writeOnly);
          const uint8_t *src = result.data.data();

          for (int y = 0; y < ch; ++y) {
            const uint8_t *srcRow = src + y * cRowPitch;
            uint8_t *dstRow = bmp.getLinePointer(y);
            for (int x = 0; x < cw; ++x) {
              // BGRA -> ARGB (JUCE ARGB format)
              dstRow[x * 4 + 0] = srcRow[x * 4 + 0]; // B
              dstRow[x * 4 + 1] = srcRow[x * 4 + 1]; // G
              dstRow[x * 4 + 2] = srcRow[x * 4 + 2]; // R
              dstRow[x * 4 + 3] = srcRow[x * 4 + 3]; // A
            }
          }

          {
            std::lock_guard<std::mutex> lock(self->frameMutex_);
            self->volumeFrame_ = std::move(img);
          }

          // Release the staging buffer after readback
          wgpuBufferRelease(staging);
        });
  }
}

void GpuComputeManager::createOffscreenTexture(WGPUDevice device, int w,
                                                int h) {
  if (offscreenView_) {
    wgpuTextureViewRelease(offscreenView_);
    offscreenView_ = nullptr;
  }
  if (offscreenTexture_) {
    wgpuTextureDestroy(offscreenTexture_);
    wgpuTextureRelease(offscreenTexture_);
    offscreenTexture_ = nullptr;
  }

  offscreenW_ = w;
  offscreenH_ = h;

  WGPUTextureDescriptor desc = {};
  desc.label = toSV("vol_offscreen");
  desc.size = {(uint32_t)w, (uint32_t)h, 1};
  desc.mipLevelCount = 1;
  desc.sampleCount = 1;
  desc.dimension = WGPUTextureDimension_2D;
  desc.format = WGPUTextureFormat_BGRA8Unorm;
  desc.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;

  offscreenTexture_ = wgpuDeviceCreateTexture(device, &desc);
  offscreenView_ = wgpuTextureCreateView(offscreenTexture_, nullptr);
}