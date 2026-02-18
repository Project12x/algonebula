// StereoChorus.h - Stereo chorus effect
// Adapted from DaisySP Chorus (MIT License, Copyright 2020 Electrosmith)
// Self-contained, header-only implementation.
#pragma once

#include "StereoEffect.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

class StereoChorus : public StereoEffect {
public:
  void init(float sampleRate) override {
    sr_ = sampleRate;
    delayBufL_.assign(kMaxDelay, 0.0f);
    delayBufR_.assign(kMaxDelay, 0.0f);
    writePos_ = 0;
    lfoPhaseL_ = 0.0f;
    lfoPhaseR_ = 0.25f;
    setRate(0.5f);
    setDepth(0.4f);
    setFeedback(0.2f);
  }

  void setRate(float hz) { lfoInc_ = hz / sr_; }
  void setDepth(float d) { depth_ = std::max(0.0f, std::min(1.0f, d)); }
  void setFeedback(float fb) {
    feedback_ = std::max(-0.9f, std::min(0.9f, fb));
  }

  void process(float inL, float inR, float &outL, float &outR) override {
    float lfoL = triangleLfo(lfoPhaseL_);
    float lfoR = triangleLfo(lfoPhaseR_);
    float centerDelay = sr_ * 0.007f;
    float modRange = sr_ * 0.003f * depth_;
    float delayL = centerDelay + lfoL * modRange;
    float delayR = centerDelay + lfoR * modRange;
    float wetL = readDelay(delayBufL_, delayL);
    float wetR = readDelay(delayBufR_, delayR);
    delayBufL_[writePos_] = sanitize(inL);
    delayBufR_[writePos_] = sanitize(inR);
    writePos_ = (writePos_ + 1) % kMaxDelay;
    lfoPhaseL_ += lfoInc_;
    if (lfoPhaseL_ >= 1.0f)
      lfoPhaseL_ -= 1.0f;
    lfoPhaseR_ += lfoInc_;
    if (lfoPhaseR_ >= 1.0f)
      lfoPhaseR_ -= 1.0f;
    float mix = getMix();
    outL = inL * (1.0f - mix) + wetL * mix;
    outR = inR * (1.0f - mix) + wetR * mix;
  }

  void reset() override {
    std::fill(delayBufL_.begin(), delayBufL_.end(), 0.0f);
    std::fill(delayBufR_.begin(), delayBufR_.end(), 0.0f);
    writePos_ = 0;
    lfoPhaseL_ = 0.0f;
    lfoPhaseR_ = 0.25f;
  }

  const char *getName() const override { return "Chorus"; }

private:
  static constexpr int kMaxDelay = 2048;

  float triangleLfo(float phase) const {
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

  float lfoInc_ = 0.0f;
  float lfoPhaseL_ = 0.0f;
  float lfoPhaseR_ = 0.25f;
  float depth_ = 0.4f;
  float feedback_ = 0.2f;
  int writePos_ = 0;
  std::vector<float> delayBufL_;
  std::vector<float> delayBufR_;
};
