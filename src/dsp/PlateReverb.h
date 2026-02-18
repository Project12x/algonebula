// PlateReverb.h - Dattorro plate reverb (public domain algorithm)
// Reference: Jon Dattorro, "Effect Design Part 1: Reverberator and Other
// Filters" (J. Audio Eng. Soc., Vol 45, No 9, 1997 September)
// Self-contained, header-only implementation.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

class PlateReverb {
public:
  void init(float sampleRate) {
    sr_ = sampleRate;
    float scale = sr_ / 29761.0f; // Dattorro's reference rate

    // Allocate and scale all delay lines
    auto makeDelay = [&](int baseSamples) -> std::vector<float> {
      int sz = std::max(1, static_cast<int>(baseSamples * scale));
      return std::vector<float>(sz, 0.0f);
    };

    // Input diffusion allpass chains
    inAP1_ = makeDelay(142);
    inAP2_ = makeDelay(107);
    inAP3_ = makeDelay(379);
    inAP4_ = makeDelay(277);

    // Tank delay lines (two parallel paths)
    tankAP1_ = makeDelay(672);
    tankDelay1_ = makeDelay(4453);
    tankAP2_ = makeDelay(1800);
    tankDelay2_ = makeDelay(3720);

    tankAP3_ = makeDelay(908);
    tankDelay3_ = makeDelay(4217);
    tankAP4_ = makeDelay(2656);
    tankDelay4_ = makeDelay(3163);

    // Pre-delay
    preDelay_ = makeDelay(static_cast<int>(0.02f * sr_)); // 20ms max

    // LFO for tank modulation
    lfoPhase_ = 0.0f;
    lfoInc_ = 1.0f / sr_; // ~1 Hz modulation

    // Initialize positions
    preDelayPos_ = 0;
    resetPositions();

    setDecay(0.7f);
    setDamping(0.5f);
    setMix(0.3f);
    setPreDelay(0.0f);
  }

  void setDecay(float d) { decay_ = std::max(0.0f, std::min(0.99f, d)); }
  void setDamping(float d) { damping_ = std::max(0.0f, std::min(1.0f, d)); }
  void setMix(float m) { mix_ = std::max(0.0f, std::min(1.0f, m)); }
  void setPreDelay(float seconds) {
    preDelaySamples_ =
        static_cast<int>(std::max(0.0f, std::min(0.02f, seconds)) * sr_);
  }

  void process(float inL, float inR, float &outL, float &outR) {
    float monoIn = (inL + inR) * 0.5f;

    // Pre-delay
    float preDelayed = readBuf(preDelay_, preDelayPos_, preDelaySamples_);
    writeBuf(preDelay_, preDelayPos_, monoIn);

    // Input diffusion: 4 cascaded allpass filters
    float diffused = preDelayed;
    diffused = allpass(inAP1_, inAP1Pos_, diffused, 0.75f);
    diffused = allpass(inAP2_, inAP2Pos_, diffused, 0.75f);
    diffused = allpass(inAP3_, inAP3Pos_, diffused, 0.625f);
    diffused = allpass(inAP4_, inAP4Pos_, diffused, 0.625f);

    // LFO for subtle modulation
    float lfo = std::sin(lfoPhase_ * 6.283185f) * 0.5f;
    lfoPhase_ += lfoInc_;
    if (lfoPhase_ >= 1.0f)
      lfoPhase_ -= 1.0f;

    // === Tank Path 1 ===
    float tank1 = diffused + tank2Out_ * decay_;
    tank1 = allpassMod(tankAP1_, tankAP1Pos_, tank1, -0.7f, lfo);
    writeBuf(tankDelay1_, tankDelay1Pos_, tank1);
    tank1 = readBuf(tankDelay1_, tankDelay1Pos_,
                    static_cast<int>(tankDelay1_.size()) - 1);
    tank1 = dampLp(tank1, damp1State_, damping_);
    tank1 *= decay_;
    tank1 = allpass(tankAP2_, tankAP2Pos_, tank1, 0.5f);
    writeBuf(tankDelay2_, tankDelay2Pos_, tank1);
    float tank1Out = readBuf(tankDelay2_, tankDelay2Pos_,
                             static_cast<int>(tankDelay2_.size()) - 1);

    // === Tank Path 2 ===
    float tank2 = diffused + tank1Out * decay_;
    tank2 = allpassMod(tankAP3_, tankAP3Pos_, tank2, -0.7f, -lfo);
    writeBuf(tankDelay3_, tankDelay3Pos_, tank2);
    tank2 = readBuf(tankDelay3_, tankDelay3Pos_,
                    static_cast<int>(tankDelay3_.size()) - 1);
    tank2 = dampLp(tank2, damp2State_, damping_);
    tank2 *= decay_;
    tank2 = allpass(tankAP4_, tankAP4Pos_, tank2, 0.5f);
    writeBuf(tankDelay4_, tankDelay4Pos_, tank2);
    tank2Out_ = readBuf(tankDelay4_, tankDelay4Pos_,
                        static_cast<int>(tankDelay4_.size()) - 1);

    // Tap outputs from multiple points for rich stereo
    float reverbL = tapOutput(tankDelay1_, tankDelay1Pos_, 0.35f) +
                    tapOutput(tankDelay1_, tankDelay1Pos_, 0.78f) -
                    tapOutput(tankAP2_, tankAP2Pos_, 0.5f) +
                    tapOutput(tankDelay3_, tankDelay3Pos_, 0.62f) -
                    tapOutput(tankDelay4_, tankDelay4Pos_, 0.45f);

    float reverbR = tapOutput(tankDelay3_, tankDelay3Pos_, 0.38f) +
                    tapOutput(tankDelay3_, tankDelay3Pos_, 0.73f) -
                    tapOutput(tankAP4_, tankAP4Pos_, 0.5f) +
                    tapOutput(tankDelay1_, tankDelay1Pos_, 0.58f) -
                    tapOutput(tankDelay2_, tankDelay2Pos_, 0.42f);

    reverbL *= 0.3f; // Scale to reasonable level
    reverbR *= 0.3f;

    // Mix
    outL = inL * (1.0f - mix_) + reverbL * mix_;
    outR = inR * (1.0f - mix_) + reverbR * mix_;
  }

  void reset() {
    auto clearBuf = [](std::vector<float> &b) {
      std::fill(b.begin(), b.end(), 0.0f);
    };
    clearBuf(preDelay_);
    clearBuf(inAP1_);
    clearBuf(inAP2_);
    clearBuf(inAP3_);
    clearBuf(inAP4_);
    clearBuf(tankAP1_);
    clearBuf(tankDelay1_);
    clearBuf(tankAP2_);
    clearBuf(tankDelay2_);
    clearBuf(tankAP3_);
    clearBuf(tankDelay3_);
    clearBuf(tankAP4_);
    clearBuf(tankDelay4_);
    damp1State_ = 0.0f;
    damp2State_ = 0.0f;
    tank2Out_ = 0.0f;
    lfoPhase_ = 0.0f;
    resetPositions();
  }

private:
  void resetPositions() {
    preDelayPos_ = 0;
    inAP1Pos_ = 0;
    inAP2Pos_ = 0;
    inAP3Pos_ = 0;
    inAP4Pos_ = 0;
    tankAP1Pos_ = 0;
    tankDelay1Pos_ = 0;
    tankAP2Pos_ = 0;
    tankDelay2Pos_ = 0;
    tankAP3Pos_ = 0;
    tankDelay3Pos_ = 0;
    tankAP4Pos_ = 0;
    tankDelay4Pos_ = 0;
  }

  // Circular buffer read
  float readBuf(const std::vector<float> &buf, int writePos, int delay) const {
    int sz = static_cast<int>(buf.size());
    int pos = (writePos - delay + sz * 2) % sz;
    return buf[pos];
  }

  // Circular buffer write and advance
  void writeBuf(std::vector<float> &buf, int &writePos, float val) {
    buf[writePos] = val;
    writePos = (writePos + 1) % static_cast<int>(buf.size());
  }

  // Allpass filter using circular buffer
  float allpass(std::vector<float> &buf, int &pos, float input, float coeff) {
    float delayed = buf[pos];
    float output = -input * coeff + delayed;
    buf[pos] = input + delayed * coeff;
    pos = (pos + 1) % static_cast<int>(buf.size());
    return output;
  }

  // Modulated allpass (reads with slight offset from LFO)
  float allpassMod(std::vector<float> &buf, int &pos, float input, float coeff,
                   float /*mod*/) {
    // Simplified: just use standard allpass (mod adds subtle detuning)
    return allpass(buf, pos, input, coeff);
  }

  // One-pole lowpass damping filter
  float dampLp(float input, float &state, float damp) const {
    state = input * (1.0f - damp) + state * damp;
    return state;
  }

  // Tap a delay buffer at a fractional position (0..1 of buffer length)
  float tapOutput(const std::vector<float> &buf, int writePos,
                  float fraction) const {
    int sz = static_cast<int>(buf.size());
    int delay = static_cast<int>(fraction * sz);
    int pos = (writePos - delay + sz * 2) % sz;
    return buf[pos];
  }

  float sr_ = 48000.0f;
  float decay_ = 0.7f;
  float damping_ = 0.5f;
  float mix_ = 0.3f;
  int preDelaySamples_ = 0;

  float damp1State_ = 0.0f;
  float damp2State_ = 0.0f;
  float tank2Out_ = 0.0f;
  float lfoPhase_ = 0.0f;
  float lfoInc_ = 0.0f;

  // Delay line buffers
  std::vector<float> preDelay_;
  std::vector<float> inAP1_, inAP2_, inAP3_, inAP4_;
  std::vector<float> tankAP1_, tankDelay1_, tankAP2_, tankDelay2_;
  std::vector<float> tankAP3_, tankDelay3_, tankAP4_, tankDelay4_;

  // Write positions
  int preDelayPos_ = 0;
  int inAP1Pos_ = 0, inAP2Pos_ = 0, inAP3Pos_ = 0, inAP4Pos_ = 0;
  int tankAP1Pos_ = 0, tankDelay1Pos_ = 0;
  int tankAP2Pos_ = 0, tankDelay2Pos_ = 0;
  int tankAP3Pos_ = 0, tankDelay3Pos_ = 0;
  int tankAP4Pos_ = 0, tankDelay4Pos_ = 0;
};
