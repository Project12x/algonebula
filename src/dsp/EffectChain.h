// EffectChain.h - Reorderable effect slot manager
// Manages an ordered array of StereoEffect pointers with per-effect
// bypass and parallel send/return architecture.
// Header-only, no external dependencies beyond StereoEffect.
#pragma once

#include "StereoEffect.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>

class EffectChain {
public:
  static constexpr int kMaxSlots = 9; // Max number of effect slots

  void init(float sampleRate) {
    sr_ = sampleRate;
    for (int i = 0; i < kMaxSlots; ++i) {
      if (slots_[i] != nullptr)
        slots_[i]->init(sampleRate);
    }
  }

  // Set an effect in a slot. nullptr clears the slot.
  void setSlot(int index, StereoEffect *effect) {
    if (index >= 0 && index < kMaxSlots)
      slots_[index] = effect;
  }

  StereoEffect *getSlot(int index) const {
    if (index >= 0 && index < kMaxSlots)
      return slots_[index];
    return nullptr;
  }

  int getSlotCount() const {
    int count = 0;
    for (int i = 0; i < kMaxSlots; ++i)
      if (slots_[i] != nullptr)
        count++;
    return count;
  }

  // Swap two slots (for reordering). Thread-safe via simple pointer swap.
  void swapSlots(int a, int b) {
    if (a >= 0 && a < kMaxSlots && b >= 0 && b < kMaxSlots) {
      StereoEffect *tmp = slots_[a];
      slots_[a] = slots_[b];
      slots_[b] = tmp;
    }
  }

  // Process using parallel send/return architecture.
  // Each active effect receives the same soft-clipped dry signal.
  // Wet returns are summed and attenuated.
  void processParallel(float dryL, float dryR, float &outL, float &outR) {
    float wetSumL = 0.0f;
    float wetSumR = 0.0f;
    int activeCount = 0;

    for (int i = 0; i < kMaxSlots; ++i) {
      StereoEffect *fx = slots_[i];
      if (fx == nullptr || fx->isBypassed() || fx->getMix() <= 0.0f)
        continue;

      float fxL, fxR;
      fx->process(dryL, dryR, fxL, fxR);

      // Extract wet-only signal and scale by mix
      wetSumL += (fxL - dryL) * fx->getMix();
      wetSumR += (fxR - dryR) * fx->getMix();
      ++activeCount;
    }

    if (activeCount == 0) {
      outL = dryL;
      outR = dryR;
      return;
    }

    // Attenuate summed wet signal to prevent level boost
    outL = dryL + wetSumL * kWetAttenuation;
    outR = dryR + wetSumR * kWetAttenuation;
  }

  // Process in series (for effects that should be chained).
  // Each effect feeds into the next.
  void processSeries(float inL, float inR, float &outL, float &outR) {
    outL = inL;
    outR = inR;
    for (int i = 0; i < kMaxSlots; ++i) {
      StereoEffect *fx = slots_[i];
      if (fx == nullptr || fx->isBypassed())
        continue;
      fx->processWithMix(outL, outR, outL, outR);
    }
  }

  void reset() {
    for (int i = 0; i < kMaxSlots; ++i) {
      if (slots_[i] != nullptr)
        slots_[i]->reset();
    }
  }

private:
  static constexpr float kWetAttenuation = 0.5f;

  float sr_ = 44100.0f;
  StereoEffect *slots_[kMaxSlots] = {};
};
