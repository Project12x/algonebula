// ModLFO.h - Modulation LFO for parameter automation
// Supports: sine, triangle, saw, square, random (sample-and-hold)
// Header-only, no external dependencies.
#pragma once

#include <cmath>
#include <cstdint>

class ModLFO {
public:
  enum Shape { Sine = 0, Triangle, Saw, Square, Random, kNumShapes };

  void init(float sampleRate) {
    sr_ = sampleRate;
    phase_ = 0.0f;
    rng_ = 12345;
    holdValue_ = 0.0f;
  }

  void setRate(float hz) { rate_ = hz; }
  void setShape(int s) {
    shape_ = (s >= 0 && s < kNumShapes) ? static_cast<Shape>(s) : Sine;
  }

  // Tick once per sample. Returns value in [-1, +1].
  float tick() {
    float out = 0.0f;
    switch (shape_) {
    case Sine:
      out = std::sin(phase_ * 6.283185307f);
      break;
    case Triangle: {
      float t = phase_ * 4.0f;
      if (t < 1.0f)
        out = t;
      else if (t < 3.0f)
        out = 2.0f - t;
      else
        out = t - 4.0f;
      break;
    }
    case Saw:
      out = 2.0f * phase_ - 1.0f;
      break;
    case Square:
      out = (phase_ < 0.5f) ? 1.0f : -1.0f;
      break;
    case Random:
      // Sample-and-hold: new random value each cycle
      if (phase_ < lastPhase_) {
        rng_ ^= rng_ << 13;
        rng_ ^= rng_ >> 7;
        rng_ ^= rng_ << 17;
        holdValue_ = (static_cast<float>(rng_ & 0xFFFF) / 32768.0f) - 1.0f;
      }
      out = holdValue_;
      break;
    default:
      break;
    }

    lastPhase_ = phase_;
    phase_ += rate_ / sr_;
    if (phase_ >= 1.0f)
      phase_ -= 1.0f;

    return out;
  }

  // Tick once per block (N samples). Advances phase by N samples.
  float tickBlock(int blockSize) {
    float out = tick();
    // Advance phase for remaining samples
    phase_ += rate_ / sr_ * static_cast<float>(blockSize - 1);
    while (phase_ >= 1.0f)
      phase_ -= 1.0f;
    return out;
  }

  void reset() {
    phase_ = 0.0f;
    lastPhase_ = 0.0f;
    holdValue_ = 0.0f;
  }

private:
  float sr_ = 44100.0f;
  float rate_ = 1.0f;
  float phase_ = 0.0f;
  float lastPhase_ = 0.0f;
  Shape shape_ = Sine;
  uint64_t rng_ = 12345;
  float holdValue_ = 0.0f;
};
