// StereoChorus.h - Stereo chorus effect
// Adapted from DaisySP Chorus (MIT License, Copyright 2020 Electrosmith)
// Self-contained, header-only implementation.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

class StereoChorus {
public:
  void init(float sampleRate) {
    sr_ = sampleRate;
    delayBufL_.assign(kMaxDelay, 0.0f);
    delayBufR_.assign(kMaxDelay, 0.0f);
    writePos_ = 0;
    lfoPhaseL_ = 0.0f;
    lfoPhaseR_ = 0.25f; // Offset for stereo width
    setRate(0.5f);
    setDepth(0.4f);
    setFeedback(0.2f);
    setMix(0.5f);
  }

  void setRate(float hz) { lfoInc_ = hz / sr_; }
  void setDepth(float d) { depth_ = std::max(0.0f, std::min(1.0f, d)); }
  void setFeedback(float fb) {
    feedback_ = std::max(-0.9f, std::min(0.9f, fb));
  }
  void setMix(float m) { mix_ = std::max(0.0f, std::min(1.0f, m)); }

  void process(float inL, float inR, float &outL, float &outR) {
    // LFO: triangle wave
    float lfoL = triangleLfo(lfoPhaseL_);
    float lfoR = triangleLfo(lfoPhaseR_);

    // Modulated delay time in samples (center around ~7ms)
    float centerDelay = sr_ * 0.007f;       // 7ms center
    float modRange = sr_ * 0.003f * depth_; // Up to 3ms modulation
    float delayL = centerDelay + lfoL * modRange;
    float delayR = centerDelay + lfoR * modRange;

    // Read from delay with linear interpolation
    float wetL = readDelay(delayBufL_, delayL);
    float wetR = readDelay(delayBufR_, delayR);

    // Write to delay (no feedback - continuous input doesn't need
    // recirculation)
    delayBufL_[writePos_] = sanitize(inL);
    delayBufR_[writePos_] = sanitize(inR);

    // Advance write position
    writePos_ = (writePos_ + 1) % kMaxDelay;

    // Advance LFO
    lfoPhaseL_ += lfoInc_;
    if (lfoPhaseL_ >= 1.0f)
      lfoPhaseL_ -= 1.0f;
    lfoPhaseR_ += lfoInc_;
    if (lfoPhaseR_ >= 1.0f)
      lfoPhaseR_ -= 1.0f;

    // Mix
    outL = inL * (1.0f - mix_) + wetL * mix_;
    outR = inR * (1.0f - mix_) + wetR * mix_;
  }

  void reset() {
    std::fill(delayBufL_.begin(), delayBufL_.end(), 0.0f);
    std::fill(delayBufR_.begin(), delayBufR_.end(), 0.0f);
    writePos_ = 0;
    lfoPhaseL_ = 0.0f;
    lfoPhaseR_ = 0.25f;
  }

private:
  static constexpr int kMaxDelay = 2048;

  // Kill denormals, NaN, Inf
  static float sanitize(float x) {
    if (std::isnan(x) || std::isinf(x))
      return 0.0f;
    if (std::fabs(x) < 1.0e-15f)
      return 0.0f;
    return std::max(-1.5f, std::min(1.5f, x));
  }

  float triangleLfo(float phase) const {
    // Phase 0..1 -> triangle -1..1
    float t = phase * 4.0f;
    if (t < 1.0f)
      return t;
    if (t < 3.0f)
      return 2.0f - t;
    return t - 4.0f;
  }

  float readDelay(const std::vector<float> &buf, float delaySamples) const {
    float pos = static_cast<float>(writePos_) - delaySamples;
    if (pos < 0.0f)
      pos += kMaxDelay;

    int i0 = static_cast<int>(pos);
    int i1 = (i0 + 1) % kMaxDelay;
    float frac = pos - static_cast<float>(i0);
    i0 = i0 % kMaxDelay;

    return buf[i0] * (1.0f - frac) + buf[i1] * frac;
  }

  float sr_ = 48000.0f;
  float lfoInc_ = 0.0f;
  float lfoPhaseL_ = 0.0f;
  float lfoPhaseR_ = 0.25f;
  float depth_ = 0.4f;
  float feedback_ = 0.2f;
  float mix_ = 0.5f;
  int writePos_ = 0;

  std::vector<float> delayBufL_;
  std::vector<float> delayBufR_;
};
