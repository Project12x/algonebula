#pragma once
// CpuStepTimer -- Runs CPU cellular engine stepping on the message thread.
// Mirrors the GPU path: audio thread sets atomic flags, timer processes them.
// This keeps engine->step() off the audio thread for RT-safety.

#include "CellEditQueue.h"
#include "CellularEngine.h"
#include "../gpu/GpuGridBridge.h"
#include <atomic>
#include <juce_events/juce_events.h>

class CpuStepTimer : private juce::Timer {
public:
  CpuStepTimer() = default;
  ~CpuStepTimer() override { stop(); }

  /// Wire up to engine and bridge. Call before start().
  void setTargets(CellularEngine *engine, GpuGridBridge *bridge,
                  CellEditQueue *editQueue) {
    engine_ = engine;
    bridge_ = bridge;
    editQueue_ = editQueue;
  }

  /// Begin stepping at ~60 FPS.
  void start() {
    if (!engine_ || !bridge_)
      return;
    startTimerHz(60);
  }

  /// Stop stepping.
  void stop() { stopTimer(); }

  // --- Atomic flags set by audio thread ---

  /// Request one step (audio thread sets on clock tick).
  void requestStep() {
    stepsRequested_.fetch_add(1, std::memory_order_relaxed);
  }

  /// Request reseed with given seed and density.
  void requestReseed(uint64_t seed, float density, bool symmetric) {
    reseedSeed_ = seed;
    reseedDensity_ = density;
    reseedSymmetric_ = symmetric;
    reseedRequested_.store(true, std::memory_order_release);
  }

  /// Request clear.
  void requestClear() {
    clearRequested_.store(true, std::memory_order_release);
  }

  /// Request overpopulation reseed (sparser).
  void requestOverpopReseed(uint64_t seed, float density, bool symmetric) {
    overpopSeed_ = seed;
    overpopDensity_ = density;
    overpopSymmetric_ = symmetric;
    overpopRequested_.store(true, std::memory_order_release);
  }

private:
  void timerCallback() override {
    if (!engine_ || !bridge_)
      return;

    // Process clear request
    if (clearRequested_.exchange(false, std::memory_order_acquire)) {
      engine_->clear();
      bridge_->updateFromCpu(engine_->getGrid());
      return;
    }

    // Process reseed request
    if (reseedRequested_.exchange(false, std::memory_order_acquire)) {
      if (reseedSymmetric_)
        engine_->randomizeSymmetric(reseedSeed_, reseedDensity_);
      else
        engine_->randomize(reseedSeed_, reseedDensity_);
      bridge_->updateFromCpu(engine_->getGrid());
      return;
    }

    // Process overpop reseed
    if (overpopRequested_.exchange(false, std::memory_order_acquire)) {
      if (overpopSymmetric_)
        engine_->randomizeSymmetric(overpopSeed_, overpopDensity_);
      else
        engine_->randomize(overpopSeed_, overpopDensity_);
      bridge_->updateFromCpu(engine_->getGrid());
      return;
    }

    // Drain cell edits from UI
    if (editQueue_) {
      editQueue_->drainInto(engine_->getGridMutable());
    }

    // Process pending steps (consume all accumulated requests as one step)
    int pending = stepsRequested_.exchange(0, std::memory_order_relaxed);
    if (pending > 0) {
      engine_->getGridMutable().snapshotPrev();
      engine_->step();
      bridge_->updateFromCpu(engine_->getGrid());
    }
  }

  CellularEngine *engine_ = nullptr;
  GpuGridBridge *bridge_ = nullptr;
  CellEditQueue *editQueue_ = nullptr;

  // Step requests (counter to handle multiple clock ticks per timer tick)
  std::atomic<int> stepsRequested_{0};

  // Reseed
  std::atomic<bool> reseedRequested_{false};
  uint64_t reseedSeed_ = 0;
  float reseedDensity_ = 0.3f;
  bool reseedSymmetric_ = false;

  // Clear
  std::atomic<bool> clearRequested_{false};

  // Overpopulation reseed
  std::atomic<bool> overpopRequested_{false};
  uint64_t overpopSeed_ = 0;
  float overpopDensity_ = 0.15f;
  bool overpopSymmetric_ = false;
};
