// TapeSaturation.h - Analog tape saturation effect
// Soft clipping waveshaper with one-pole lowpass for warmth.
// Header-only, no external dependencies.
#pragma once

#include "StereoEffect.h"
#include <algorithm>
#include <cmath>

class TapeSaturation : public StereoEffect {
public:
  void init(float sampleRate) override {
    sr_ = sampleRate;
    lpStateL_ = 0.0f;
    lpStateR_ = 0.0f;
    setDrive(0.5f);
    setTone(0.7f);
  }

  // Drive: 0.0 (clean) to 1.0 (heavy saturation)
  void setDrive(float d) { drive_ = std::max(0.0f, std::min(1.0f, d)); }

  // Tone: 0.0 (dark, heavy LP) to 1.0 (bright, minimal LP)
  void setTone(float t) {
    tone_ = std::max(0.0f, std::min(1.0f, t));
    // Map tone to lowpass coefficient (higher = brighter)
    float freq = 2000.0f + tone_ * 18000.0f; // 2kHz to 20kHz
    float w = 2.0f * 3.141592653f * freq / sr_;
    lpCoeff_ = w / (1.0f + w);
  }

  void process(float inL, float inR, float &outL, float &outR) override {
    // Apply drive (gain + waveshaper)
    float gainDb = drive_ * 24.0f; // Up to 24dB of drive
    float gain = std::pow(10.0f, gainDb / 20.0f);
    float xL = inL * gain;
    float xR = inR * gain;

    // Soft clipping: tanh approximation (Pade 3/2)
    xL = tanhApprox(xL);
    xR = tanhApprox(xR);

    // One-pole lowpass for tape warmth
    lpStateL_ += lpCoeff_ * (xL - lpStateL_);
    lpStateR_ += lpCoeff_ * (xR - lpStateR_);

    // Compensate output level (drive adds gain, waveshaper reduces it
    // partially)
    float compensation = 1.0f / std::max(1.0f, gain * 0.5f);
    outL = sanitize(lpStateL_ * compensation);
    outR = sanitize(lpStateR_ * compensation);
  }

  void reset() override {
    lpStateL_ = 0.0f;
    lpStateR_ = 0.0f;
  }

  const char *getName() const override { return "Tape Sat"; }

private:
  // Fast tanh approximation (Pade 3/2)
  static float tanhApprox(float x) {
    if (x > 3.0f)
      return 1.0f;
    if (x < -3.0f)
      return -1.0f;
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
  }

  float drive_ = 0.5f;
  float tone_ = 0.7f;
  float lpCoeff_ = 0.5f;
  float lpStateL_ = 0.0f;
  float lpStateR_ = 0.0f;
};
