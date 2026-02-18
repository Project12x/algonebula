// PingPongDelay.h - Stereo ping-pong delay
// Alternating L/R delay taps with lowpass-filtered feedback.
// Header-only, no external dependencies.
#pragma once

#include "StereoEffect.h"
#include <algorithm>
#include <cmath>
#include <vector>

class PingPongDelay : public StereoEffect {
public:
  void init(float sampleRate) override {
    sr_ = sampleRate;
    maxDelaySamples_ = static_cast<int>(sr_ * kMaxDelaySec) + 2;
    bufL_.assign(maxDelaySamples_, 0.0f);
    bufR_.assign(maxDelaySamples_, 0.0f);
    writePosL_ = 0;
    writePosR_ = 0;
    lpStateL_ = 0.0f;
    lpStateR_ = 0.0f;
    setTime(0.375f); // Dotted 8th at 120 BPM
    setFeedback(0.4f);
    setTone(0.6f);
    setWidth(1.0f);
  }

  // Delay time in seconds
  void setTime(float seconds) {
    float s = std::max(0.01f, std::min(kMaxDelaySec, seconds));
    delaySamples_ = s * sr_;
  }

  // Feedback: 0 to 0.85
  void setFeedback(float fb) {
    feedback_ = std::max(0.0f, std::min(0.85f, fb));
  }

  // Tone: lowpass in feedback path (0=dark, 1=bright)
  void setTone(float t) {
    tone_ = std::max(0.0f, std::min(1.0f, t));
    float freq = 500.0f + tone_ * 15000.0f; // 500Hz to 15.5kHz
    float w = 2.0f * 3.141592653f * freq / sr_;
    lpCoeff_ = w / (1.0f + w);
  }

  // Stereo width: 0=mono, 1=full ping-pong
  void setWidth(float w) { width_ = std::max(0.0f, std::min(1.0f, w)); }

  void process(float inL, float inR, float &outL, float &outR) override {
    // Read delayed samples
    float tapL = readDelay(bufL_, writePosL_, delaySamples_);
    float tapR = readDelay(bufR_, writePosR_, delaySamples_);

    // Lowpass filter in feedback path
    lpStateL_ += lpCoeff_ * (tapR - lpStateL_); // R feeds back to L
    lpStateR_ += lpCoeff_ * (tapL - lpStateR_); // L feeds back to R

    // Write: input + cross-feedback (ping-pong pattern)
    // No self-feedback — only cross-feed for ping-pong effect
    bufL_[writePosL_] = sanitize(inL + lpStateL_ * feedback_);
    bufR_[writePosR_] = sanitize(inR + lpStateR_ * feedback_);

    writePosL_ = (writePosL_ + 1) % maxDelaySamples_;
    writePosR_ = (writePosR_ + 1) % maxDelaySamples_;

    // Width: blend between mono delays and stereo ping-pong
    float monoTap = (tapL + tapR) * 0.5f;
    float widthL = monoTap * (1.0f - width_) + tapL * width_;
    float widthR = monoTap * (1.0f - width_) + tapR * width_;

    outL = sanitize(inL + widthL);
    outR = sanitize(inR + widthR);
  }

  void reset() override {
    std::fill(bufL_.begin(), bufL_.end(), 0.0f);
    std::fill(bufR_.begin(), bufR_.end(), 0.0f);
    writePosL_ = 0;
    writePosR_ = 0;
    lpStateL_ = 0.0f;
    lpStateR_ = 0.0f;
  }

  const char *getName() const override { return "Ping Pong"; }

private:
  static constexpr float kMaxDelaySec = 2.0f;

  float readDelay(const std::vector<float> &buf, int writePos,
                  float delaySamples) const {
    float pos = static_cast<float>(writePos) - delaySamples;
    if (pos < 0.0f)
      pos += maxDelaySamples_;
    int i0 = static_cast<int>(pos);
    int i1 = (i0 + 1) % maxDelaySamples_;
    float frac = pos - static_cast<float>(i0);
    i0 = i0 % maxDelaySamples_;
    return buf[i0] * (1.0f - frac) + buf[i1] * frac;
  }

  float delaySamples_ = 0.0f;
  float feedback_ = 0.4f;
  float tone_ = 0.6f;
  float width_ = 1.0f;
  float lpCoeff_ = 0.5f;
  float lpStateL_ = 0.0f;
  float lpStateR_ = 0.0f;
  int maxDelaySamples_ = 96000;
  int writePosL_ = 0;
  int writePosR_ = 0;
  std::vector<float> bufL_;
  std::vector<float> bufR_;
};
