#pragma once

#include <cmath>
#include <cstdint>

/// Clock divider for tempo-synced engine stepping.
/// Supports host transport sync and free-running internal clock.
/// Uses integer sample counting (no floating-point drift).
/// RT-safe: no allocations, no logging in tick path.
class ClockDivider {
public:
  /// Clock division values (relative to quarter note at given BPM).
  enum class Division : int {
    Whole = 0,    // 1/1
    Half,         // 1/2
    Quarter,      // 1/4
    Eighth,       // 1/8
    Sixteenth,    // 1/16
    ThirtySecond, // 1/32
    Count
  };

  /// Reset clock state. Call in prepareToPlay().
  void reset(double sampleRate) {
    sr = sampleRate;
    sampleCounter = 0;
    stepReady = false;
    isOddStep = false;
    recalcStepSamples();
  }

  /// Set BPM (from host transport or manual).
  void setBPM(double bpm) {
    if (bpm < 20.0)
      bpm = 20.0;
    if (bpm > 300.0)
      bpm = 300.0;
    currentBPM = bpm;
    recalcStepSamples();
  }

  /// Set clock division.
  void setDivision(Division div) {
    currentDivision = div;
    recalcStepSamples();
  }

  /// Set swing amount (50 = no swing, 75 = max).
  /// Swing offsets every other step.
  void setSwing(float swingPercent) {
    if (swingPercent < 50.0f)
      swingPercent = 50.0f;
    if (swingPercent > 75.0f)
      swingPercent = 75.0f;
    swing = swingPercent;
    recalcStepSamples();
  }

  /// Process a single sample. Returns true if a step should fire.
  /// Call once per sample in processBlock.
  bool tick() {
    ++sampleCounter;

    int64_t threshold = isOddStep ? swungStepSamples : normalStepSamples;

    if (sampleCounter >= threshold) {
      sampleCounter = 0;
      stepReady = true;
      isOddStep = !isOddStep;
      return true;
    }

    stepReady = false;
    return false;
  }

  /// Process N samples at once. Returns number of steps fired.
  /// More efficient for block-level processing.
  int processBlock(int numSamples) {
    int steps = 0;
    for (int i = 0; i < numSamples; ++i) {
      if (tick())
        ++steps;
    }
    return steps;
  }

  /// Check if a step was fired on the last tick.
  bool isStepReady() const { return stepReady; }

  /// Get the step duration in samples (for the current even/odd step).
  int64_t getStepSamples(bool odd) const {
    return odd ? swungStepSamples : normalStepSamples;
  }

  /// Get samples per quarter note at current BPM.
  int64_t getSamplesPerQuarter() const {
    if (currentBPM <= 0.0)
      return static_cast<int64_t>(sr);
    return static_cast<int64_t>(sr * 60.0 / currentBPM);
  }

  /// Get steps per second at current settings.
  double getStepsPerSecond() const {
    double quarterPerSec = currentBPM / 60.0;
    return quarterPerSec * getDivisionMultiplier();
  }

  /// Get step interval in seconds (inverse of steps per second).
  double getStepIntervalSeconds() const {
    double sps = getStepsPerSecond();
    return (sps > 0.0) ? 1.0 / sps : 0.5;
  }

  double getBPM() const { return currentBPM; }
  Division getDivision() const { return currentDivision; }
  float getSwing() const { return swing; }

private:
  void recalcStepSamples() {
    if (sr <= 0.0 || currentBPM <= 0.0) {
      normalStepSamples = 22050; // Fallback ~0.5s at 44.1kHz
      swungStepSamples = 22050;
      return;
    }

    double samplesPerQuarter = sr * 60.0 / currentBPM;
    double divMultiplier = getDivisionMultiplier();
    double baseSamples = samplesPerQuarter / divMultiplier;

    // Apply swing: swing 50% = even, swing 67% = 2:1 ratio
    // swing is percentage (50-75), where 50 = no swing
    double swingRatio = swing / 100.0;

    // Even step duration = base * 2 * swingRatio
    // Odd step duration  = base * 2 * (1 - swingRatio)
    // At swing=50: both = base
    // At swing=67: even = 1.34 * base, odd = 0.66 * base
    normalStepSamples = static_cast<int64_t>(baseSamples * 2.0 * swingRatio);
    swungStepSamples =
        static_cast<int64_t>(baseSamples * 2.0 * (1.0 - swingRatio));

    // Ensure minimum 1 sample
    if (normalStepSamples < 1)
      normalStepSamples = 1;
    if (swungStepSamples < 1)
      swungStepSamples = 1;
  }

  double getDivisionMultiplier() const {
    switch (currentDivision) {
    case Division::Whole:
      return 0.25; // 1 step per whole note
    case Division::Half:
      return 0.5; // 1 step per half note
    case Division::Quarter:
      return 1.0; // 1 step per quarter
    case Division::Eighth:
      return 2.0; // 2 steps per quarter
    case Division::Sixteenth:
      return 4.0; // 4 steps per quarter
    case Division::ThirtySecond:
      return 8.0; // 8 steps per quarter
    default:
      return 1.0;
    }
  }

  double sr = 44100.0;
  double currentBPM = 120.0;
  Division currentDivision = Division::Quarter;
  float swing = 50.0f; // 50 = no swing

  int64_t sampleCounter = 0;
  int64_t normalStepSamples = 22050;
  int64_t swungStepSamples = 22050;
  bool stepReady = false;
  bool isOddStep = false;
};
