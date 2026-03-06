#pragma once
// GpuComputeManager -- Manages GPU compute simulation lifecycle.
// Owns the simulation adapters and readback pipeline.
// Runs on the UI/message thread (via timer). Audio thread only reads
// the GpuGridBridge.

#include "GpuGridBridge.h"
#include "engine/CellularEngine.h"
#include <ghostsun_render/ComputeSimulation.h>
#include <juce_events/juce_events.h>
#include <memory>

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

  /// Access the bridge (audio thread reads this).
  GpuGridBridge &getBridge() { return bridge_; }
  const GpuGridBridge &getBridge() const { return bridge_; }

  /// Seed the simulation with random initial state.
  void seed(uint64_t rngSeed, float density);

  /// Clear the simulation state.
  void clearState();

  /// Steps per timer tick. Default 1.
  void setStepsPerFrame(int steps) { stepsPerFrame_ = std::max(1, steps); }

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

  // Simulation
  std::unique_ptr<ghostsun::ComputeSimulation> simulation_;
  EngineType currentType_ = EngineType::GoL;
  int rows_ = 0;
  int cols_ = 0;

  // Bridge
  GpuGridBridge bridge_;
};
