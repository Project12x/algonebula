// ShimmerReverb.h - Reverb with pitch-shifted feedback (+1 octave)
// FDN reverb core + octave-up pitch shift in the feedback path.
// Header-only, no external dependencies.
#pragma once

#include "StereoEffect.h"
#include <algorithm>
#include <cmath>
#include <vector>

class ShimmerReverb : public StereoEffect {
public:
  void init(float sampleRate) override {
    sr_ = sampleRate;

    // FDN delay lines (4 channels, prime-number lengths for density)
    static const int baseLengths[kChannels] = {1087, 1283, 1481, 1699};
    float scale = sr_ / 44100.0f;
    for (int i = 0; i < kChannels; ++i) {
      int len = static_cast<int>(baseLengths[i] * scale);
      if (len < 1)
        len = 1;
      delayLen_[i] = len;
      delay_[i].assign(len, 0.0f);
      delayPos_[i] = 0;
      dampState_[i] = 0.0f;
    }

    // Pitch shifter buffer (granular, ~50ms window)
    pitchBufLen_ = static_cast<int>(sr_ * 0.05f);
    if (pitchBufLen_ < 64)
      pitchBufLen_ = 64;
    pitchBufL_.assign(pitchBufLen_, 0.0f);
    pitchBufR_.assign(pitchBufLen_, 0.0f);
    pitchWritePos_ = 0;
    pitchReadPhase_ = 0.0;

    setDecay(0.8f);
    setDamping(0.5f);
    setShimmer(0.3f);
  }

  void setDecay(float d) { decay_ = std::max(0.0f, std::min(0.95f, d)); }
  void setDamping(float d) { damping_ = std::max(0.0f, std::min(0.99f, d)); }

  // Shimmer: amount of octave-up signal fed back (0=pure reverb, 1=full
  // shimmer)
  void setShimmer(float s) { shimmer_ = std::max(0.0f, std::min(1.0f, s)); }

  void process(float inL, float inR, float &outL, float &outR) override {
    // Read from FDN delay lines
    float tap[kChannels];
    for (int i = 0; i < kChannels; ++i) {
      tap[i] = delay_[i][delayPos_[i]];
      // One-pole damping
      dampState_[i] += (1.0f - damping_) * (tap[i] - dampState_[i]);
      tap[i] = dampState_[i];
    }

    // Hadamard-like mixing matrix (simplified: rotate + scale)
    float mixed[kChannels];
    mixed[0] = (tap[0] + tap[1] + tap[2] + tap[3]) * 0.5f;
    mixed[1] = (tap[0] - tap[1] + tap[2] - tap[3]) * 0.5f;
    mixed[2] = (tap[0] + tap[1] - tap[2] - tap[3]) * 0.5f;
    mixed[3] = (tap[0] - tap[1] - tap[2] + tap[3]) * 0.5f;

    // Pitch shift: octave up via granular read at 2x speed
    float shiftedL = pitchShiftRead(pitchBufL_);
    float shiftedR = pitchShiftRead(pitchBufR_);

    // Blend shimmer into feedback (shimmer amount controls ratio)
    float monoIn = (inL + inR) * 0.5f;
    float fbL = mixed[0] + mixed[1];
    float fbR = mixed[2] + mixed[3];

    // Shimmer: mix octave-up into feedback
    float shimFbL = fbL * (1.0f - shimmer_) + shiftedL * shimmer_;
    float shimFbR = fbR * (1.0f - shimmer_) + shiftedR * shimmer_;

    // Write to pitch shifter buffer (the reverb output, for next-frame shimmer)
    pitchBufL_[pitchWritePos_] = sanitize(fbL);
    pitchBufR_[pitchWritePos_] = sanitize(fbR);
    pitchWritePos_ = (pitchWritePos_ + 1) % pitchBufLen_;
    pitchReadPhase_ += 2.0; // Read at 2x speed = +1 octave
    if (pitchReadPhase_ >= static_cast<double>(pitchBufLen_))
      pitchReadPhase_ -= static_cast<double>(pitchBufLen_);

    // Write back to FDN with input injection
    for (int i = 0; i < kChannels; ++i) {
      float fb = (i < 2) ? shimFbL : shimFbR;
      float input = monoIn + fb * decay_;
      delay_[i][delayPos_[i]] = sanitize(input);
      delayPos_[i] = (delayPos_[i] + 1) % delayLen_[i];
    }

    // Output taps
    outL = sanitize(fbL * 0.4f);
    outR = sanitize(fbR * 0.4f);
  }

  void reset() override {
    for (int i = 0; i < kChannels; ++i) {
      std::fill(delay_[i].begin(), delay_[i].end(), 0.0f);
      delayPos_[i] = 0;
      dampState_[i] = 0.0f;
    }
    std::fill(pitchBufL_.begin(), pitchBufL_.end(), 0.0f);
    std::fill(pitchBufR_.begin(), pitchBufR_.end(), 0.0f);
    pitchWritePos_ = 0;
    pitchReadPhase_ = 0.0;
  }

  const char *getName() const override { return "Shimmer"; }

private:
  static constexpr int kChannels = 4;

  // Granular pitch shift read: read at 2x speed with crossfade window
  float pitchShiftRead(const std::vector<float> &buf) const {
    int len = pitchBufLen_;
    double phase = pitchReadPhase_;
    // Two grains, 180 degrees apart, with Hann crossfade
    float g1 = grainRead(buf, phase, len);
    float g2 = grainRead(buf, phase + len * 0.5, len);

    // Crossfade based on position within grain
    float pos1 = static_cast<float>(std::fmod(phase, len * 0.5) / (len * 0.5));
    float win1 = 0.5f - 0.5f * std::cos(pos1 * 6.283185307f);
    float win2 = 1.0f - win1;

    return g1 * win1 + g2 * win2;
  }

  static float grainRead(const std::vector<float> &buf, double phase, int len) {
    double p = std::fmod(phase, static_cast<double>(len));
    if (p < 0.0)
      p += len;
    int i0 = static_cast<int>(p);
    int i1 = (i0 + 1) % len;
    float frac = static_cast<float>(p - i0);
    return buf[i0] * (1.0f - frac) + buf[i1] * frac;
  }

  float decay_ = 0.8f;
  float damping_ = 0.5f;
  float shimmer_ = 0.3f;

  // FDN
  std::vector<float> delay_[kChannels];
  int delayLen_[kChannels] = {};
  int delayPos_[kChannels] = {};
  float dampState_[kChannels] = {};

  // Pitch shifter
  std::vector<float> pitchBufL_;
  std::vector<float> pitchBufR_;
  int pitchBufLen_ = 2048;
  int pitchWritePos_ = 0;
  double pitchReadPhase_ = 0.0;
};
