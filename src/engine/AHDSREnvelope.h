#pragma once

#include <cmath>
#include <cstdint>

/// AHDSR envelope generator: Attack -> Hold -> Decay -> Sustain -> Release.
/// All transitions are linear ramps. RT-safe, no allocations.
/// Retrigger from current level to avoid clicks.
class AHDSREnvelope {
public:
  enum class Stage : int { Idle = 0, Attack, Hold, Decay, Sustain, Release };

  /// Set envelope times (seconds) and sustain level (0-1).
  void setParameters(double attackTime, double holdTime, double decayTime,
                     double sustainLevel, double releaseTime,
                     double sampleRate) {
    // Convert times to per-sample increments
    double sr = sampleRate > 0.0 ? sampleRate : 44100.0;

    attackIncrement = attackTime > 0.0 ? 1.0 / (attackTime * sr) : 1.0;
    holdSamples = static_cast<int64_t>(holdTime * sr);
    decayIncrement = decayTime > 0.0 ? 1.0 / (decayTime * sr) : 1.0;
    sustain = sustainLevel;
    releaseIncrement = releaseTime > 0.0 ? 1.0 / (releaseTime * sr) : 1.0;
  }

  /// Trigger note-on. Retriggers from current level if already active.
  void noteOn() {
    stage = Stage::Attack;
    holdCounter = 0;
    // Don't reset level — retrigger from current position to avoid click
  }

  /// Trigger note-off. Transitions to release from current level.
  void noteOff() {
    if (stage != Stage::Idle) {
      stage = Stage::Release;
    }
  }

  /// Generate next envelope value. Returns 0.0-1.0.
  double nextSample() {
    switch (stage) {
    case Stage::Idle:
      return 0.0;

    case Stage::Attack:
      level += attackIncrement;
      if (level >= 1.0) {
        level = 1.0;
        stage = Stage::Hold;
        holdCounter = 0;
      }
      return level;

    case Stage::Hold:
      ++holdCounter;
      if (holdCounter >= holdSamples) {
        stage = Stage::Decay;
      }
      return level; // stays at 1.0

    case Stage::Decay:
      level -= decayIncrement;
      if (level <= sustain) {
        level = sustain;
        stage = Stage::Sustain;
      }
      return level;

    case Stage::Sustain:
      return level; // holds at sustain level

    case Stage::Release:
      level -= releaseIncrement;
      if (level <= 0.0) {
        level = 0.0;
        stage = Stage::Idle;
      }
      return level;
    }

    return 0.0;
  }

  /// Check if envelope is still producing output.
  bool isActive() const { return stage != Stage::Idle; }

  /// Get current stage.
  Stage getStage() const { return stage; }

  /// Get current level (for diagnostics).
  double getLevel() const { return level; }

  /// Force reset to idle.
  void reset() {
    stage = Stage::Idle;
    level = 0.0;
    holdCounter = 0;
  }

private:
  Stage stage = Stage::Idle;
  double level = 0.0;

  double attackIncrement = 0.0;
  int64_t holdSamples = 0;
  int64_t holdCounter = 0;
  double decayIncrement = 0.0;
  double sustain = 0.7;
  double releaseIncrement = 0.0;
};
