// StereoDelay.h - Stereo delay with cross-feedback
// Adapted from DaisySP DelayLine template (MIT License, Copyright 2020
// Electrosmith) Self-contained, header-only implementation.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

class StereoDelay {
public:
  void init(float sampleRate) {
    sr_ = sampleRate;
    maxDelaySamples_ = static_cast<int>(sr_ * kMaxDelaySec);
    bufL_.assign(maxDelaySamples_, 0.0f);
    bufR_.assign(maxDelaySamples_, 0.0f);
    writePos_ = 0;
    setTime(0.3f); // 300ms default
    setFeedback(0.4f);
    setCrossFeed(0.2f); // Slight ping-pong
    setMix(0.3f);
  }

  // Delay time in seconds
  void setTime(float seconds) {
    float s = std::max(0.001f, std::min(kMaxDelaySec, seconds));
    delaySamples_ = s * sr_;
  }

  void setFeedback(float fb) {
    feedback_ = std::max(0.0f, std::min(0.75f, fb));
    clampTotalFeedback();
  }
  void setCrossFeed(float cf) {
    crossFeed_ = std::max(0.0f, std::min(0.5f, cf));
    clampTotalFeedback();
  }
  void setMix(float m) { mix_ = std::max(0.0f, std::min(1.0f, m)); }

  void process(float inL, float inR, float &outL, float &outR) {
    // Read from delay with linear interpolation
    float wetL = readDelay(bufL_, delaySamples_);
    float wetR = readDelay(bufR_, delaySamples_);

    // Write with cross-feedback (total feedback clamped < 1.0)
    float writeL = inL + wetL * feedback_ + wetR * crossFeed_;
    float writeR = inR + wetR * feedback_ + wetL * crossFeed_;

    // Anti-denormal + hard clamp
    writeL = sanitize(writeL);
    writeR = sanitize(writeR);

    bufL_[writePos_] = writeL;
    bufR_[writePos_] = writeR;

    // Advance write position
    writePos_ = (writePos_ + 1) % maxDelaySamples_;

    // Mix
    outL = inL * (1.0f - mix_) + wetL * mix_;
    outR = inR * (1.0f - mix_) + wetR * mix_;
  }

  void reset() {
    std::fill(bufL_.begin(), bufL_.end(), 0.0f);
    std::fill(bufR_.begin(), bufR_.end(), 0.0f);
    writePos_ = 0;
  }

private:
  static constexpr float kMaxDelaySec = 2.0f;
  static constexpr float kMaxTotalFeedback = 0.92f; // Must be < 1.0

  // Ensure feedback + crossfeed don't exceed safe limit
  void clampTotalFeedback() {
    float total = feedback_ + crossFeed_;
    if (total > kMaxTotalFeedback) {
      float scale = kMaxTotalFeedback / total;
      feedback_ *= scale;
      crossFeed_ *= scale;
    }
  }

  // Kill denormals, NaN, Inf, and clamp to safe range
  static float sanitize(float x) {
    if (std::isnan(x) || std::isinf(x))
      return 0.0f;
    // Kill denormals
    if (std::fabs(x) < 1.0e-15f)
      return 0.0f;
    // Hard clamp to prevent runaway
    return std::max(-4.0f, std::min(4.0f, x));
  }

  float readDelay(const std::vector<float> &buf, float delaySamples) const {
    float pos = static_cast<float>(writePos_) - delaySamples;
    if (pos < 0.0f)
      pos += maxDelaySamples_;

    int i0 = static_cast<int>(pos);
    int i1 = (i0 + 1) % maxDelaySamples_;
    float frac = pos - static_cast<float>(i0);
    i0 = i0 % maxDelaySamples_;

    return buf[i0] * (1.0f - frac) + buf[i1] * frac;
  }

  float sr_ = 48000.0f;
  int maxDelaySamples_ = 96000;
  float delaySamples_ = 14400.0f;
  float feedback_ = 0.4f;
  float crossFeed_ = 0.2f;
  float mix_ = 0.3f;
  int writePos_ = 0;

  std::vector<float> bufL_;
  std::vector<float> bufR_;
};
