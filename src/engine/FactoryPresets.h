#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <map>
#include <string>
#include <vector>

/// Lightweight factory preset system.
/// Each preset is a name + map of paramID -> raw float value.
struct FactoryPreset {
  const char *name;
  const char *category; // "Musical" or "Experimental"
  std::map<std::string, float> values;

  /// Apply this preset to an APVTS (message thread only).
  void apply(juce::AudioProcessorValueTreeState &apvts) const {
    for (const auto &[id, val] : values) {
      if (auto *param = apvts.getParameter(id)) {
        param->setValueNotifyingHost(param->convertTo0to1(val));
      }
    }
  }
};

/// Returns all factory presets. Call once and cache.
inline std::vector<FactoryPreset> getFactoryPresets() {
  return {
      // --- 0: Init (all defaults) ---
      {"Init",
       "Utility",
       {{"algorithm", 0.0f},
        {"scale", 1.0f},
        {"key", 0.0f},
        {"waveshape", 0.0f},
        {"bpm", 120.0f},
        {"clockDiv", 2.0f},
        {"swing", 50.0f},
        {"attack", 0.5f},
        {"hold", 0.0f},
        {"decay", 0.5f},
        {"sustain", 0.7f},
        {"release", 3.0f},
        {"filterCutoff", 8000.0f},
        {"filterRes", 0.0f},
        {"filterMode", 0.0f},
        {"noiseLevel", 0.0f},
        {"subLevel", 0.0f},
        {"subOctave", 0.0f},
        {"masterVolume", 0.7f},
        {"voiceCount", 4.0f},
        {"melodicInertia", 0.3f},
        {"roundRobin", 0.2f},
        {"strumSpread", 0.0f},
        {"velocityHumanize", 0.05f},
        {"droneSustain", 0.0f},
        {"noteProbability", 1.0f},
        {"gateTime", 0.8f},
        {"tuning", 0.0f},
        {"refPitch", 440.0f}}},

      // --- 1: Reversed Violin ---
      {"Reversed Violin",
       "Musical",
       {{"algorithm", 0.0f},
        {"scale", 7.0f},     // Aeolian
        {"key", 7.0f},       // G
        {"waveshape", 6.0f}, // Pad
        {"bpm", 120.0f},
        {"clockDiv", 3.0f}, // 1/8
        {"swing", 62.5f},
        {"attack", 1.789f}, // Long swell
        {"hold", 0.654f},
        {"decay", 0.001f},   // Instant
        {"sustain", 0.0f},   // No sustain
        {"release", 0.004f}, // Instant
        {"filterCutoff", 3447.5f},
        {"filterRes", 0.56f},
        {"filterMode", 0.0f}, // LP
        {"noiseLevel", 0.1f},
        {"subLevel", 0.5f},
        {"subOctave", 0.0f}, // -1 Oct
        {"masterVolume", 0.647f},
        {"voiceCount", 4.0f},
        {"melodicInertia", 0.61f},
        {"roundRobin", 0.55f},
        {"strumSpread", 25.9f},
        {"velocityHumanize", 0.15f},
        {"droneSustain", 0.0f},
        {"noteProbability", 1.0f},
        {"gateTime", 0.8f},
        {"tuning", 0.0f},
        {"refPitch", 440.0f}}},

      // --- 2: Crystalline Bells ---
      {"Crystalline Bells",
       "Musical",
       {{"algorithm", 0.0f},
        {"scale", 1.0f},     // Major
        {"key", 0.0f},       // C
        {"waveshape", 7.0f}, // Bell
        {"bpm", 80.0f},
        {"clockDiv", 3.0f}, // 1/8
        {"swing", 50.0f},
        {"attack", 0.005f},
        {"hold", 0.1f},
        {"decay", 2.5f},
        {"sustain", 0.0f},
        {"release", 4.0f},
        {"filterCutoff", 12000.0f},
        {"filterRes", 0.2f},
        {"filterMode", 0.0f},
        {"noiseLevel", 0.0f},
        {"subLevel", 0.0f},
        {"subOctave", 0.0f},
        {"masterVolume", 0.6f},
        {"voiceCount", 6.0f},
        {"melodicInertia", 0.5f},
        {"roundRobin", 0.3f},
        {"strumSpread", 5.0f},
        {"velocityHumanize", 0.1f},
        {"droneSustain", 0.0f},
        {"noteProbability", 0.7f},
        {"gateTime", 0.5f},
        {"tuning", 0.0f},
        {"refPitch", 440.0f}}},

      // --- 3: Dark Drone ---
      {"Dark Drone",
       "Experimental",
       {{"algorithm", 4.0f}, // Ambient rule
        {"scale", 2.0f},     // Minor
        {"key", 2.0f},       // D
        {"waveshape", 2.0f}, // Saw
        {"bpm", 60.0f},
        {"clockDiv", 0.0f}, // 1/1
        {"swing", 50.0f},
        {"attack", 3.0f},
        {"hold", 1.0f},
        {"decay", 5.0f},
        {"sustain", 0.8f},
        {"release", 8.0f},
        {"filterCutoff", 800.0f},
        {"filterRes", 0.7f},
        {"filterMode", 0.0f},
        {"noiseLevel", 0.05f},
        {"subLevel", 0.8f},
        {"subOctave", 1.0f}, // -2 Oct
        {"masterVolume", 0.5f},
        {"voiceCount", 3.0f},
        {"melodicInertia", 0.9f},
        {"roundRobin", 0.1f},
        {"strumSpread", 0.0f},
        {"velocityHumanize", 0.0f},
        {"droneSustain", 0.8f},
        {"noteProbability", 0.3f},
        {"gateTime", 1.0f},
        {"tuning", 0.0f},
        {"refPitch", 440.0f}}},

      // --- 4: Pulsing Seeds ---
      {"Pulsing Seeds",
       "Experimental",
       {{"algorithm", 3.0f}, // Seeds rule
        {"scale", 0.0f},     // Chromatic
        {"key", 0.0f},       // C
        {"waveshape", 3.0f}, // Pulse
        {"bpm", 140.0f},
        {"clockDiv", 4.0f}, // 1/16
        {"swing", 67.0f},
        {"attack", 0.001f},
        {"hold", 0.0f},
        {"decay", 0.2f},
        {"sustain", 0.0f},
        {"release", 0.1f},
        {"filterCutoff", 5000.0f},
        {"filterRes", 0.4f},
        {"filterMode", 0.0f},
        {"noiseLevel", 0.15f},
        {"subLevel", 0.0f},
        {"subOctave", 0.0f},
        {"masterVolume", 0.55f},
        {"voiceCount", 8.0f},
        {"melodicInertia", 0.0f},
        {"roundRobin", 0.8f},
        {"strumSpread", 0.0f},
        {"velocityHumanize", 0.2f},
        {"droneSustain", 0.0f},
        {"noteProbability", 1.0f},
        {"gateTime", 0.3f},
        {"tuning", 0.0f},
        {"refPitch", 440.0f}}},

      // --- 5: Ethereal Fifths ---
      {"Ethereal Fifths",
       "Musical",
       {{"algorithm", 0.0f},
        {"scale", 9.0f},     // Pent. Major
        {"key", 5.0f},       // F
        {"waveshape", 5.0f}, // Fifth Stack
        {"bpm", 90.0f},
        {"clockDiv", 2.0f}, // 1/4
        {"swing", 55.0f},
        {"attack", 1.2f},
        {"hold", 0.3f},
        {"decay", 2.0f},
        {"sustain", 0.4f},
        {"release", 5.0f},
        {"filterCutoff", 6000.0f},
        {"filterRes", 0.3f},
        {"filterMode", 0.0f},
        {"noiseLevel", 0.03f},
        {"subLevel", 0.3f},
        {"subOctave", 0.0f},
        {"masterVolume", 0.6f},
        {"voiceCount", 4.0f},
        {"melodicInertia", 0.7f},
        {"roundRobin", 0.4f},
        {"strumSpread", 15.0f},
        {"velocityHumanize", 0.1f},
        {"droneSustain", 0.0f},
        {"noteProbability", 0.8f},
        {"gateTime", 0.7f},
        {"tuning", 1.0f}, // Just Intonation
        {"refPitch", 440.0f}}},
  };
}
