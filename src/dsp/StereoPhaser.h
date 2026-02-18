// StereoPhaser.h - Stereo phaser effect
// 4-stage allpass cascade with LFO-swept cutoff.
// Header-only, no external dependencies.
#pragma once

#include "StereoEffect.h"
#include <algorithm>
#include <cmath>
#include <vector>

class StereoPhaser : public StereoEffect {
public:
  void init(float sampleRate) override {
    sr_ = sampleRate;
    lfoPhase_ = 0.0f;
    setRate(0.4f);
    setDepth(0.7f);
    setFeedback(0.3f);
    for (int i = 0; i < kStages; ++i) {
      apStateL_[i] = 0.0f;
      apStateR_[i] = 0.0f;
    }
    lastOutL_ = 0.0f;
    lastOutR_ = 0.0f;
  }

  void setRate(float hz) {
    lfoInc_ = std::max(0.01f, std::min(10.0f, hz)) / sr_;
  }
  void setDepth(float d) { depth_ = std::max(0.0f, std::min(1.0f, d)); }
  void setFeedback(float fb) {
    feedback_ = std::max(-0.85f, std::min(0.85f, fb));
  }

  void process(float inL, float inR, float &outL, float &outR) override {
    // LFO: sine wave, stereo offset
    float lfo = std::sin(lfoPhase_ * 6.283185307f);
    float lfoR = std::sin((lfoPhase_ + 0.25f) * 6.283185307f);
    lfoPhase_ += lfoInc_;
    if (lfoPhase_ >= 1.0f)
      lfoPhase_ -= 1.0f;

    // Map LFO to allpass coefficient range (200Hz - 4kHz)
    float minFreq = 200.0f;
    float maxFreq = std::min(4000.0f, sr_ * 0.45f);
    float freqL = minFreq + (maxFreq - minFreq) * (0.5f + 0.5f * lfo * depth_);
    float freqR = minFreq + (maxFreq - minFreq) * (0.5f + 0.5f * lfoR * depth_);
    float coeffL = allpassCoeff(freqL);
    float coeffR = allpassCoeff(freqR);

    // Feed input + feedback from last output
    float xL = sanitize(inL + lastOutL_ * feedback_);
    float xR = sanitize(inR + lastOutR_ * feedback_);

    // 4-stage allpass cascade
    for (int i = 0; i < kStages; ++i) {
      xL = allpassProcess(xL, apStateL_[i], coeffL);
      xR = allpassProcess(xR, apStateR_[i], coeffR);
    }

    lastOutL_ = xL;
    lastOutR_ = xR;

    // Output: notch-style interference pattern
    outL = sanitize(inL + xL);
    outR = sanitize(inR + xR);
  }

  void reset() override {
    for (int i = 0; i < kStages; ++i) {
      apStateL_[i] = 0.0f;
      apStateR_[i] = 0.0f;
    }
    lastOutL_ = 0.0f;
    lastOutR_ = 0.0f;
    lfoPhase_ = 0.0f;
  }

  const char *getName() const override { return "Phaser"; }

private:
  static constexpr int kStages = 4;

  // First-order allpass coefficient from frequency
  float allpassCoeff(float freq) const {
    float t = std::tan(3.141592653f * freq / sr_);
    return (t - 1.0f) / (t + 1.0f);
  }

  // First-order allpass filter
  static float allpassProcess(float input, float &state, float coeff) {
    float y = coeff * input + state;
    state = input - coeff * y;
    return y;
  }

  float lfoInc_ = 0.0f;
  float lfoPhase_ = 0.0f;
  float depth_ = 0.7f;
  float feedback_ = 0.3f;
  float apStateL_[kStages] = {};
  float apStateR_[kStages] = {};
  float lastOutL_ = 0.0f;
  float lastOutR_ = 0.0f;
};
