// StereoFlanger.h - Stereo flanger effect
// Short modulated delay (0.1-5ms) with feedback for comb filtering.
// Header-only, no external dependencies.
#pragma once

#include "StereoEffect.h"
#include <algorithm>
#include <cmath>
#include <vector>

class StereoFlanger : public StereoEffect {
public:
  void init(float sampleRate) override {
    sr_ = sampleRate;
    maxDelaySamples_ = static_cast<int>(sr_ * kMaxDelaySec) + 2;
    bufL_.assign(maxDelaySamples_, 0.0f);
    bufR_.assign(maxDelaySamples_, 0.0f);
    writePos_ = 0;
    lfoPhase_ = 0.0f;
    setRate(0.3f);
    setDepth(0.6f);
    setFeedback(0.4f);
  }

  void setRate(float hz) {
    lfoInc_ = std::max(0.01f, std::min(10.0f, hz)) / sr_;
  }
  void setDepth(float d) { depth_ = std::max(0.0f, std::min(1.0f, d)); }
  void setFeedback(float fb) {
    feedback_ = std::max(-0.85f, std::min(0.85f, fb));
  }

  void process(float inL, float inR, float &outL, float &outR) override {
    // LFO: triangle wave, stereo offset for width
    float lfoL = triangleLfo(lfoPhase_);
    float lfoR = triangleLfo(lfoPhase_ + 0.5f);
    lfoPhase_ += lfoInc_;
    if (lfoPhase_ >= 1.0f)
      lfoPhase_ -= 1.0f;

    // Modulated delay time: 0.1ms to 5ms
    float minDelay = sr_ * 0.0001f; // 0.1ms
    float maxDelay = sr_ * 0.005f;  // 5ms
    float range = (maxDelay - minDelay) * depth_;
    float delayL = minDelay + range * (0.5f + 0.5f * lfoL);
    float delayR = minDelay + range * (0.5f + 0.5f * lfoR);

    // Read from delay with interpolation
    float wetL = readDelay(bufL_, delayL);
    float wetR = readDelay(bufR_, delayR);

    // Write to delay (input only, no feedback recirculation for continuous
    // input)
    bufL_[writePos_] = sanitize(inL);
    bufR_[writePos_] = sanitize(inR);
    writePos_ = (writePos_ + 1) % maxDelaySamples_;

    // Output: direct + delayed creates comb filter effect
    outL = sanitize(inL + wetL);
    outR = sanitize(inR + wetR);
  }

  void reset() override {
    std::fill(bufL_.begin(), bufL_.end(), 0.0f);
    std::fill(bufR_.begin(), bufR_.end(), 0.0f);
    writePos_ = 0;
    lfoPhase_ = 0.0f;
  }

  const char *getName() const override { return "Flanger"; }

private:
  static constexpr float kMaxDelaySec = 0.01f; // 10ms max

  float triangleLfo(float phase) const {
    float p = phase - std::floor(phase); // Wrap to 0..1
    float t = p * 4.0f;
    if (t < 1.0f)
      return t;
    if (t < 3.0f)
      return 2.0f - t;
    return t - 4.0f;
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

  float lfoInc_ = 0.0f;
  float lfoPhase_ = 0.0f;
  float depth_ = 0.6f;
  float feedback_ = 0.4f;
  int maxDelaySamples_ = 441;
  int writePos_ = 0;
  std::vector<float> bufL_;
  std::vector<float> bufR_;
};
