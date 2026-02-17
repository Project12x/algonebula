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
      // --- 0: Init (musical defaults) ---
      {"Init",
       "Utility",
       {{"algorithm", 0.0f},
        {"scale", 1.0f},
        {"key", 0.0f},
        {"waveshape", 0.0f},
        {"bpm", 120.0f},
        {"clockDiv", 2.0f},
        {"swing", 50.0f},
        {"attack", 0.8f},
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
        {"voiceCount", 3.0f},
        {"melodicInertia", 0.5f},
        {"roundRobin", 0.2f},
        {"strumSpread", 0.0f},
        {"velocityHumanize", 0.05f},
        {"droneSustain", 0.0f},
        {"noteProbability", 0.5f},
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
        {"noteProbability", 0.6f},
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
        {"noteProbability", 0.5f},
        {"gateTime", 0.5f},
        {"tuning", 0.0f},
        {"refPitch", 440.0f}}},

      // --- 3: Dark Drone ---
      {"Dark Drone",
       "Musical",
       {{"algorithm", 0.0f}, // GoL (classic, slow evolution)
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

      // --- 4: Pulsing Seeds (fast percussive, GoL) ---
      {"Pulsing Seeds",
       "Experimental",
       {{"algorithm", 0.0f}, // GoL (classic, high-energy)
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
        {"voiceCount", 4.0f},
        {"melodicInertia", 0.0f},
        {"roundRobin", 0.8f},
        {"strumSpread", 0.0f},
        {"velocityHumanize", 0.2f},
        {"droneSustain", 0.0f},
        {"noteProbability", 0.4f},
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
        {"noteProbability", 0.5f},
        {"gateTime", 0.7f},
        {"tuning", 1.0f}, // Just Intonation
        {"refPitch", 440.0f}}},

      // --- 6: Nebula Drift (blending pad) ---
      {"Nebula Drift",
       "Musical",
       {{"algorithm", 0.0f}, // Classic GoL
        {"scale", 9.0f},     // Pent. Major (consonant)
        {"key", 0.0f},       // C
        {"waveshape", 6.0f}, // Pad
        {"bpm", 60.0f},      // Slow
        {"clockDiv", 1.0f},  // 1/2 note
        {"swing", 50.0f},
        {"attack", 2.5f}, // Long fade-in (notes blend)
        {"hold", 1.0f},
        {"decay", 4.0f},    // Gentle fade
        {"sustain", 0.6f},  // Hold tone
        {"release", 10.0f}, // Long tail (overlap into next)
        {"filterCutoff", 2500.0f},
        {"filterRes", 0.35f},
        {"filterMode", 0.0f}, // LP
        {"noiseLevel", 0.02f},
        {"subLevel", 0.4f},
        {"subOctave", 0.0f}, // -1 Oct
        {"masterVolume", 0.55f},
        {"voiceCount", 3.0f},
        {"melodicInertia", 0.85f}, // Strong pitch memory
        {"roundRobin", 0.3f},
        {"strumSpread", 20.0f},
        {"velocityHumanize", 0.1f},
        {"droneSustain", 0.7f},     // Overlapping tones
        {"noteProbability", 0.35f}, // Sparse triggers
        {"gateTime", 1.0f},         // Full gate
        {"tuning", 0.0f},
        {"refPitch", 440.0f},
        {"symmetry", 1.0f}}}, // 4-fold mirror

      // --- 7: Tidal Lenia (slow continuous waves) ---
      {"Tidal Lenia",
       "Musical",
       {{"algorithm", 6.0f}, // Lenia
        {"scale", 9.0f},     // Pent. Major
        {"key", 7.0f},       // G
        {"waveshape", 6.0f}, // Pad
        {"bpm", 72.0f},
        {"clockDiv", 1.0f}, // 1/2
        {"swing", 50.0f},
        {"attack", 2.0f},
        {"hold", 0.5f},
        {"decay", 3.0f},
        {"sustain", 0.5f},
        {"release", 6.0f},
        {"filterCutoff", 3000.0f},
        {"filterRes", 0.4f},
        {"filterMode", 0.0f}, // LP
        {"noiseLevel", 0.02f},
        {"subLevel", 0.5f},
        {"subOctave", 0.0f}, // -1 Oct
        {"masterVolume", 0.55f},
        {"voiceCount", 3.0f},
        {"melodicInertia", 0.8f},
        {"roundRobin", 0.2f},
        {"strumSpread", 10.0f},
        {"velocityHumanize", 0.15f},
        {"droneSustain", 0.5f},
        {"noteProbability", 0.35f},
        {"gateTime", 0.9f},
        {"tuning", 0.0f},
        {"refPitch", 440.0f},
        {"gridSize", 2.0f}}}, // Large grid for Lenia patterns

      // --- 8: Chemical Garden (evolving R-D textures) ---
      {"Chemical Garden",
       "Experimental",
       {{"algorithm", 4.0f}, // Reaction-Diffusion
        {"scale", 13.0f},    // Harmonic Minor
        {"key", 9.0f},       // A
        {"waveshape", 4.0f}, // Sine+Oct
        {"bpm", 100.0f},
        {"clockDiv", 2.0f}, // 1/4
        {"swing", 50.0f},
        {"attack", 1.0f},
        {"hold", 0.2f},
        {"decay", 2.0f},
        {"sustain", 0.3f},
        {"release", 4.0f},
        {"filterCutoff", 4500.0f},
        {"filterRes", 0.5f},
        {"filterMode", 0.0f}, // LP
        {"noiseLevel", 0.04f},
        {"subLevel", 0.3f},
        {"subOctave", 0.0f},
        {"masterVolume", 0.55f},
        {"voiceCount", 3.0f},
        {"melodicInertia", 0.6f},
        {"roundRobin", 0.5f},
        {"strumSpread", 8.0f},
        {"velocityHumanize", 0.12f},
        {"droneSustain", 0.3f},
        {"noteProbability", 0.4f},
        {"gateTime", 0.7f},
        {"tuning", 0.0f},
        {"refPitch", 440.0f},
        {"gridSize", 2.0f}}}, // Large grid for R-D patterns

      // --- 9: Neural Flicker (Brian's Brain 3-state pulse) ---
      {"Neural Flicker",
       "Experimental",
       {{"algorithm", 2.0f}, // Brian's Brain
        {"scale", 0.0f},     // Chromatic
        {"key", 0.0f},       // C
        {"waveshape", 3.0f}, // Pulse
        {"bpm", 130.0f},
        {"clockDiv", 3.0f}, // 1/8
        {"swing", 58.0f},
        {"attack", 0.002f},
        {"hold", 0.0f},
        {"decay", 0.15f},
        {"sustain", 0.0f},
        {"release", 0.3f},
        {"filterCutoff", 6500.0f},
        {"filterRes", 0.45f},
        {"filterMode", 0.0f}, // LP
        {"noiseLevel", 0.08f},
        {"subLevel", 0.0f},
        {"subOctave", 0.0f},
        {"masterVolume", 0.5f},
        {"voiceCount", 5.0f},
        {"melodicInertia", 0.2f},
        {"roundRobin", 0.7f},
        {"strumSpread", 3.0f},
        {"velocityHumanize", 0.18f},
        {"droneSustain", 0.0f},
        {"noteProbability", 0.45f},
        {"gateTime", 0.25f},
        {"tuning", 0.0f},
        {"refPitch", 440.0f}}},

      // --- 10: Spectrum Cycle (Cyclic CA color wheel) ---
      {"Spectrum Cycle",
       "Experimental",
       {{"algorithm", 3.0f}, // Cyclic CA
        {"scale", 12.0f},    // Whole Tone
        {"key", 4.0f},       // E
        {"waveshape", 4.0f}, // Sine+Oct
        {"bpm", 108.0f},
        {"clockDiv", 2.0f}, // 1/4
        {"swing", 50.0f},
        {"attack", 0.5f},
        {"hold", 0.2f},
        {"decay", 1.5f},
        {"sustain", 0.3f},
        {"release", 3.0f},
        {"filterCutoff", 5500.0f},
        {"filterRes", 0.3f},
        {"filterMode", 0.0f}, // LP
        {"noiseLevel", 0.0f},
        {"subLevel", 0.2f},
        {"subOctave", 0.0f},
        {"masterVolume", 0.55f},
        {"voiceCount", 4.0f},
        {"melodicInertia", 0.4f},
        {"roundRobin", 0.5f},
        {"strumSpread", 12.0f},
        {"velocityHumanize", 0.08f},
        {"droneSustain", 0.2f},
        {"noteProbability", 0.5f},
        {"gateTime", 0.6f},
        {"tuning", 0.0f},
        {"refPitch", 440.0f},
        {"gridSize", 1.0f}}}, // Medium

      // --- 11: Swarm Murmuration (flowing particle trails) ---
      {"Swarm Murmuration",
       "Musical",
       {{"algorithm", 5.0f}, // Particle Swarm
        {"scale", 10.0f},    // Pent. Minor
        {"key", 9.0f},       // A
        {"waveshape", 2.0f}, // Saw
        {"bpm", 84.0f},
        {"clockDiv", 2.0f}, // 1/4
        {"swing", 54.0f},
        {"attack", 1.5f},
        {"hold", 0.3f},
        {"decay", 2.5f},
        {"sustain", 0.4f},
        {"release", 5.0f},
        {"filterCutoff", 3500.0f},
        {"filterRes", 0.5f},
        {"filterMode", 0.0f}, // LP
        {"noiseLevel", 0.03f},
        {"subLevel", 0.4f},
        {"subOctave", 0.0f},
        {"masterVolume", 0.55f},
        {"voiceCount", 3.0f},
        {"melodicInertia", 0.7f},
        {"roundRobin", 0.3f},
        {"strumSpread", 18.0f},
        {"velocityHumanize", 0.12f},
        {"droneSustain", 0.4f},
        {"noteProbability", 0.4f},
        {"gateTime", 0.8f},
        {"tuning", 1.0f}, // Just Intonation
        {"refPitch", 440.0f},
        {"gridSize", 2.0f}}}, // Large

      // --- 12: Fog Machine (diffuse Brownian drones) ---
      {"Fog Machine",
       "Musical",
       {{"algorithm", 7.0f}, // Brownian Field
        {"scale", 9.0f},     // Pent. Major
        {"key", 5.0f},       // F
        {"waveshape", 6.0f}, // Pad
        {"bpm", 55.0f},
        {"clockDiv", 0.0f}, // 1/1 (very slow)
        {"swing", 50.0f},
        {"attack", 4.0f},
        {"hold", 1.0f},
        {"decay", 6.0f},
        {"sustain", 0.7f},
        {"release", 10.0f},
        {"filterCutoff", 1800.0f},
        {"filterRes", 0.4f},
        {"filterMode", 0.0f}, // LP
        {"noiseLevel", 0.06f},
        {"subLevel", 0.6f},
        {"subOctave", 1.0f}, // -2 Oct
        {"masterVolume", 0.5f},
        {"voiceCount", 2.0f},
        {"melodicInertia", 0.95f},
        {"roundRobin", 0.1f},
        {"strumSpread", 0.0f},
        {"velocityHumanize", 0.05f},
        {"droneSustain", 0.9f},
        {"noteProbability", 0.25f},
        {"gateTime", 1.0f},
        {"tuning", 2.0f}, // Pythagorean
        {"refPitch", 440.0f},
        {"gridSize", 2.0f}}}, // Large for diffuse patterns
  };
}
