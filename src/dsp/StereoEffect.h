// StereoEffect.h - Abstract base class for all stereo effects
// Provides common interface for EffectChain slot management.
#pragma once

#include <cmath>

class StereoEffect {
public:
  virtual ~StereoEffect() = default;

  virtual void init(float sampleRate) = 0;
  virtual void process(float inL, float inR, float &outL, float &outR) = 0;
  virtual void reset() = 0;
  virtual const char *getName() const = 0;

  void setMix(float m) { mix_ = std::max(0.0f, std::min(1.0f, m)); }
  float getMix() const { return mix_; }

  void setBypass(bool b) { bypassed_ = b; }
  bool isBypassed() const { return bypassed_; }

  // Convenience: process with bypass and mix handling
  void processWithMix(float inL, float inR, float &outL, float &outR) {
    if (bypassed_ || mix_ <= 0.0f) {
      outL = inL;
      outR = inR;
      return;
    }
    float wetL, wetR;
    process(inL, inR, wetL, wetR);
    if (mix_ >= 1.0f) {
      outL = wetL;
      outR = wetR;
    } else {
      outL = inL * (1.0f - mix_) + wetL * mix_;
      outR = inR * (1.0f - mix_) + wetR * mix_;
    }
  }

protected:
  float sr_ = 44100.0f;

  // Kill denormals, NaN, Inf. Clamp to safe range.
  static float sanitize(float x) {
    if (std::isnan(x) || std::isinf(x))
      return 0.0f;
    if (std::fabs(x) < 1.0e-15f)
      return 0.0f;
    return std::max(-1.5f, std::min(1.5f, x));
  }

private:
  float mix_ = 1.0f;
  bool bypassed_ = false;
};
