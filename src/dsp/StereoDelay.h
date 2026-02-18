// StereoDelay.h - Stereo delay effect
// Adapted from DaisySP DelayLine template (MIT License, Copyright 2020
// Electrosmith) Self-contained, header-only implementation.
#pragma once

#include "StereoEffect.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

class StereoDelay : public StereoEffect {
public:
  void init(float sampleRate) override {
    sr_ = sampleRate;
    maxDelaySamples_ = static_cast<int>(sr_ * kMaxDelaySec);
    bufL_.assign(maxDelaySamples_, 0.0f);
    bufR_.assign(maxDelaySamples_, 0.0f);
    writePos_ = 0;
    setTime(0.3f);
    setFeedback(0.4f);
    setCrossFeed(0.2f);
  }

  void setTime(float seconds) {
    float s = std::max(0.001f, std::min(kMaxDelaySec, seconds));
    delaySamples_ = s * sr_;
  }

  void setFeedback(float fb) {
    feedback_ = std::max(0.0f, std::min(0.75f, fb));
    clampTotalFeedback();
  }

  void setCrossFeed(float cf) {
    crossFeed_ = std::max(0.0f, std::min(0.25f, cf));
    clampTotalFeedback();
  }

  void process(float inL, float inR, float &outL, float &outR) override {
    float wetL = readDelay(bufL_, delaySamples_);
    float wetR = readDelay(bufR_, delaySamples_);
    bufL_[writePos_] = sanitize(inL);
    bufR_[writePos_] = sanitize(inR);
    writePos_ = (writePos_ + 1) % maxDelaySamples_;
    float mix = getMix();
    outL = inL * (1.0f - mix) + wetL * mix;
    outR = inR * (1.0f - mix) + wetR * mix;
  }

  void reset() override {
    std::fill(bufL_.begin(), bufL_.end(), 0.0f);
    std::fill(bufR_.begin(), bufR_.end(), 0.0f);
    writePos_ = 0;
  }

  const char *getName() const override { return "Delay"; }

private:
  static constexpr float kMaxDelaySec = 2.0f;
  static constexpr float kMaxTotalFeedback = 0.75f;

  void clampTotalFeedback() {
    float total = feedback_ + crossFeed_;
    if (total > kMaxTotalFeedback) {
      float scale = kMaxTotalFeedback / total;
      feedback_ *= scale;
      crossFeed_ *= scale;
    }
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

  int maxDelaySamples_ = 96000;
  float delaySamples_ = 14400.0f;
  float feedback_ = 0.4f;
  float crossFeed_ = 0.2f;
  int writePos_ = 0;
  std::vector<float> bufL_;
  std::vector<float> bufR_;
};
