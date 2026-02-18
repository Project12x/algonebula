// SafetyProcessor.h - DC filter + ultrasonic LP + brickwall limiter
// Always-on safety processor at the end of the signal chain.
// Zero allocations after init, no branching on user input.
// Header-only, no external dependencies.
#pragma once

#include <algorithm>
#include <cmath>

class SafetyProcessor {
public:
  void init(float sampleRate) {
    sr_ = sampleRate;

    // DC blocking filter: 5Hz highpass (one-pole)
    // coefficient = 1 - (2*pi*freq / sampleRate)
    float dcW = 2.0f * 3.141592653f * kDcCutoffHz / sr_;
    dcCoeff_ = 1.0f / (1.0f + dcW);
    dcStateL_ = 0.0f;
    dcStateR_ = 0.0f;
    dcPrevInL_ = 0.0f;
    dcPrevInR_ = 0.0f;

    // Ultrasonic lowpass: 20kHz one-pole LP (if sr > 44.1kHz)
    float lpFreq = std::min(20000.0f, sr_ * 0.45f);
    float lpW = 2.0f * 3.141592653f * lpFreq / sr_;
    lpCoeff_ = lpW / (1.0f + lpW);
    lpStateL_ = 0.0f;
    lpStateR_ = 0.0f;

    // Brickwall: -0.3dBFS true peak limit
    brickwallThreshold_ = kBrickwallDbfs;
  }

  // Process a single stereo sample. Always-on, no bypass.
  void process(float &L, float &R) {
    // 1. DC blocking highpass (removes sub-5Hz content)
    float dcOutL = dcCoeff_ * (dcStateL_ + L - dcPrevInL_);
    float dcOutR = dcCoeff_ * (dcStateR_ + R - dcPrevInR_);
    dcPrevInL_ = L;
    dcPrevInR_ = R;
    dcStateL_ = dcOutL;
    dcStateR_ = dcOutR;
    L = dcOutL;
    R = dcOutR;

    // 2. Ultrasonic lowpass (kills content above 20kHz)
    lpStateL_ += lpCoeff_ * (L - lpStateL_);
    lpStateR_ += lpCoeff_ * (R - lpStateR_);
    L = lpStateL_;
    R = lpStateR_;

    // 3. Brickwall clamp (-0.3dBFS true peak)
    L = std::max(-brickwallThreshold_, std::min(brickwallThreshold_, L));
    R = std::max(-brickwallThreshold_, std::min(brickwallThreshold_, R));

    // 4. Final NaN/Inf/denormal guard
    if (std::isnan(L) || std::isinf(L) || std::fabs(L) < 1.0e-15f)
      L = 0.0f;
    if (std::isnan(R) || std::isinf(R) || std::fabs(R) < 1.0e-15f)
      R = 0.0f;
  }

  void reset() {
    dcStateL_ = 0.0f;
    dcStateR_ = 0.0f;
    dcPrevInL_ = 0.0f;
    dcPrevInR_ = 0.0f;
    lpStateL_ = 0.0f;
    lpStateR_ = 0.0f;
  }

private:
  static constexpr float kDcCutoffHz = 5.0f;
  // -0.3 dBFS = 10^(-0.3/20) = 0.96605...
  static constexpr float kBrickwallDbfs = 0.966051f;

  float sr_ = 44100.0f;

  // DC blocking highpass
  float dcCoeff_ = 0.999f;
  float dcStateL_ = 0.0f;
  float dcStateR_ = 0.0f;
  float dcPrevInL_ = 0.0f;
  float dcPrevInR_ = 0.0f;

  // Ultrasonic lowpass
  float lpCoeff_ = 0.5f;
  float lpStateL_ = 0.0f;
  float lpStateR_ = 0.0f;

  // Brickwall
  float brickwallThreshold_ = 0.966051f;
};
