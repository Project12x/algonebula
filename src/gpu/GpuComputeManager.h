#pragma once
// GpuComputeManager -- Manages GPU compute simulation lifecycle.
// Owns the simulation adapters and readback pipeline.
// Runs on the UI/message thread (via timer). Audio thread only reads
// the GpuGridBridge.

#include "GpuGridBridge.h"
#include "engine/CellularEngine.h"
#include <atomic>
#include <chrono>
#include <ghostsun_render/ComputeSimulation.h>
#include <ghostsun_render/ReadbackManager.h>
#include <ghostsun_render/VolumeRenderer.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <memory>
#include <mutex>

class GpuComputeManager : private juce::Timer {
public:
  GpuComputeManager() = default;
  ~GpuComputeManager() override { stop(); }

  GpuComputeManager(const GpuComputeManager &) = delete;
  GpuComputeManager &operator=(const GpuComputeManager &) = delete;

  /// Set the engine type and grid dimensions.
  /// Creates the appropriate ComputeSimulation adapter.
  /// Call from UI thread only.
  bool setEngine(EngineType type, int rows, int cols);

  /// Start the GPU simulation loop (timer-driven, ~60 FPS).
  /// Returns false if device/simulation not ready.
  bool start() {
    if (!simulation_ || !deviceReady_)
      return false;
    running_ = true;
    startTimer(16); // ~60 FPS
    return true;
  }

  /// Stop the simulation loop and release GPU resources.
  void stop() {
    running_ = false;
    stopTimer();
    shutdownGpu();
  }

  /// Whether the GPU simulation is actively running.
  bool isRunning() const { return running_ && deviceReady_; }

  /// GPU step time in milliseconds (smoothed).
  float getGpuStepMs() const {
    return gpuStepMs_.load(std::memory_order_relaxed);
  }

  /// Access the bridge (audio thread reads this).
  GpuGridBridge &getBridge() { return bridge_; }
  const GpuGridBridge &getBridge() const { return bridge_; }

  /// Seed the simulation with random initial state.
  void seed(uint64_t rngSeed, float density);

  /// Clear the simulation state.
  void clearState();

  /// Steps per timer tick. Default 1.
  void setStepsPerFrame(int steps) { stepsPerFrame_ = std::max(1, steps); }

  // --- 3D Volume rendering ---

  /// Whether the current engine is 3D (Lenia3D).
  bool is3D() const { return currentType_ == EngineType::Lenia3D; }

  /// Get the latest rendered volume frame as a JUCE Image.
  /// Thread-safe; called from the UI paint thread.
  juce::Image getVolumeFrame() const {
    std::lock_guard<std::mutex> lock(frameMutex_);
    return volumeFrame_;
  }

  /// Camera orbit angle (radians). Set from UI mouse drag.
  void setCameraOrbit(float radians) { cameraOrbit_ = radians; }
  void setCameraElevation(float elev) { cameraElevation_ = elev; }
  void setCameraDistance(float dist) { cameraDistance_ = dist; }
  void setColorMode(int mode) { colorMode_ = mode; }
  void setAudioLevel(float level) { audioLevel_ = level; }
  float getCameraOrbit() const { return cameraOrbit_; }

private:
  void timerCallback() override;

  /// Initialize GPU device (lazy, first use).
  bool ensureDevice();

  /// Shutdown GPU resources.
  void shutdownGpu();

  // GPU state
  bool deviceReady_ = false;
  bool running_ = false;
  int stepsPerFrame_ = 1;
  int timerIntervalMs_ = 16;

  // Simulation
  std::unique_ptr<ghostsun::ComputeSimulation> simulation_;
  EngineType currentType_ = EngineType::GoL;
  int rows_ = 0;
  int cols_ = 0;

  // Readback (library-managed lifecycle)
  ghostsun::ReadbackManager readbackMgr_;

  // Bridge
  GpuGridBridge bridge_;
  std::atomic<float> gpuStepMs_{0.0f};

  // 3D Volume rendering
  ghostsun::VolumeRenderer volumeRenderer_;
  bool volumeRendererReady_ = false;
  mutable std::mutex frameMutex_;
  juce::Image volumeFrame_;
  float cameraOrbit_ = 0.0f;
  float cameraElevation_ = 0.3f;
  float cameraDistance_ = 2.0f;
  int colorMode_ = 3; // nebula
  float audioLevel_ = 0.0f;
  float time_ = 0.0f;

  // Volume pixel readback
  ghostsun::ReadbackManager pixelReadbackMgr_;
  WGPUTexture offscreenTexture_ = nullptr;
  WGPUTextureView offscreenView_ = nullptr;
  int offscreenW_ = 0, offscreenH_ = 0;

  void renderVolumeFrame(WGPUDevice device, WGPUCommandEncoder encoder);
  void createOffscreenTexture(WGPUDevice device, int w, int h);
};
