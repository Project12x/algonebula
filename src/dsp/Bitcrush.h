// Bitcrush.h - Bit depth and sample rate reduction effect
// Creates lo-fi, retro digital artifacts.
// Header-only, no external dependencies.
#pragma once

#include "StereoEffect.h"
#include <algorithm>
#include <cmath>

class Bitcrush : public StereoEffect {
public:
  void init(float sampleRate) override {
    sr_ = sampleRate;
    holdL_ = 0.0f;
    holdR_ = 0.0f;
    holdCounter_ = 0.0f;
    setBitDepth(8.0f);
    setDownsample(1.0f);
  }

  // Bit depth: 1.0 (extreme crush) to 16.0 (clean)
  void setBitDepth(float bits) {
    bitDepth_ = std::max(1.0f, std::min(16.0f, bits));
    levels_ = std::pow(2.0f, bitDepth_);
  }

  // Downsample factor: 1.0 (no reduction) to 50.0 (extreme)
  void setDownsample(float factor) {
    downsampleFactor_ = std::max(1.0f, std::min(50.0f, factor));
  }

  void process(float inL, float inR, float &outL, float &outR) override {
    // Sample rate reduction via sample-and-hold
    holdCounter_ += 1.0f;
    if (holdCounter_ >= downsampleFactor_) {
      holdCounter_ -= downsampleFactor_;
      // Bit depth reduction: quantize to N levels
      holdL_ = std::floor(inL * levels_) / levels_;
      holdR_ = std::floor(inR * levels_) / levels_;
    }
    outL = sanitize(holdL_);
    outR = sanitize(holdR_);
  }

  void reset() override {
    holdL_ = 0.0f;
    holdR_ = 0.0f;
    holdCounter_ = 0.0f;
  }

  const char *getName() const override { return "Bitcrush"; }

private:
  float bitDepth_ = 8.0f;
  float levels_ = 256.0f;
  float downsampleFactor_ = 1.0f;
  float holdL_ = 0.0f;
  float holdR_ = 0.0f;
  float holdCounter_ = 0.0f;
};
