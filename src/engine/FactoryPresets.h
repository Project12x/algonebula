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
  // FX toggle helper: all off by default.
  // Presets explicitly enable what they use.
  // Toggle IDs: chorusOn, delayOn, reverbOn, phaserOn, flangerOn,
  //             bitcrushOn, tapeOn, shimmerOn, pingPongOn
  // Toggle value: 1.0f = on, 0.0f = off

  return {
      // ================================================================
      // 0: INIT — clean starting point, no effects
      // ================================================================
      {"Init",
       "Utility",
       {{"algorithm", 0.0f}, // GoL
        {"scale", 1.0f},     // Major
        {"key", 0.0f},       // C
        {"waveshape", 0.0f}, // Sine
        {"bpm", 120.0f},
        {"clockDiv", 2.0f}, // 1/4
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
        {"refPitch", 440.0f},
        {"stereoWidth", 0.5f},
        {"consonance", 0.5f},
        {"pitchGravity", 0.3f},
        {"restProbability", 0.2f},
        {"maxTriggersPerStep", 3.0f},
        {"triggerBudget", 0.0f},
        // All FX off
        {"chorusOn", 0.0f},
        {"delayOn", 0.0f},
        {"reverbOn", 0.0f},
        {"phaserOn", 0.0f},
        {"flangerOn", 0.0f},
        {"bitcrushOn", 0.0f},
        {"tapeOn", 0.0f},
        {"shimmerOn", 0.0f},
        {"pingPongOn", 0.0f},
        {"chorusMix", 0.0f},
        {"delayMix", 0.0f},
        {"reverbMix", 0.0f},
        {"phaserMix", 0.0f},
        {"flangerMix", 0.0f},
        {"bitcrushMix", 0.0f},
        {"tapeMix", 0.0f},
        {"shimmerMix", 0.0f},
        {"pingPongMix", 0.0f}}},

      // ================================================================
      // 1: Reversed Violin — slow swell, warm chorus, long reverb tail
      // ================================================================
      {"Reversed Violin",
       "Musical",
       {{"algorithm", 0.0f}, // GoL Classic
        {"scale", 7.0f},     // Aeolian
        {"key", 7.0f},       // G
        {"waveshape", 6.0f}, // Pad
        {"bpm", 120.0f},
        {"clockDiv", 3.0f}, // 1/8
        {"swing", 62.0f},
        {"attack", 1.8f}, // Long swell (reversed feel)
        {"hold", 0.65f},
        {"decay", 0.001f}, // Instant drop
        {"sustain", 0.0f},
        {"release", 0.004f},
        {"filterCutoff", 3400.0f},
        {"filterRes", 0.55f},
        {"filterMode", 0.0f}, // LP
        {"noiseLevel", 0.08f},
        {"subLevel", 0.5f},
        {"subOctave", 0.0f}, // -1 Oct
        {"masterVolume", 0.6f},
        {"voiceCount", 4.0f},
        {"melodicInertia", 0.65f},
        {"roundRobin", 0.55f},
        {"strumSpread", 25.0f},
        {"velocityHumanize", 0.15f},
        {"droneSustain", 0.0f},
        {"noteProbability", 0.55f},
        {"gateTime", 0.8f},
        {"consonance", 0.7f},
        {"pitchGravity", 0.4f},
        {"restProbability", 0.15f},
        {"maxTriggersPerStep", 3.0f},
        {"tuning", 0.0f},
        {"refPitch", 440.0f},
        {"stereoWidth", 0.6f},
        // FX: Chorus + Reverb + Tape warmth
        {"chorusOn", 1.0f},
        {"chorusRate", 0.7f},
        {"chorusDepth", 0.45f},
        {"chorusMix", 0.25f},
        {"reverbOn", 1.0f},
        {"reverbDecay", 0.65f},
        {"reverbDamping", 0.5f},
        {"reverbMix", 0.3f},
        {"tapeOn", 1.0f},
        {"tapeDrive", 0.25f},
        {"tapeTone", 0.6f},
        {"tapeMix", 0.2f},
        // Off
        {"delayOn", 0.0f},
        {"phaserOn", 0.0f},
        {"flangerOn", 0.0f},
        {"bitcrushOn", 0.0f},
        {"shimmerOn", 0.0f},
        {"pingPongOn", 0.0f}}},

      // ================================================================
      // 2: Crystalline Bells — bright attack, shimmer reverb
      // ================================================================
      {"Crystalline Bells",
       "Musical",
       {{"algorithm", 0.0f}, // GoL
        {"scale", 1.0f},     // Major
        {"key", 0.0f},       // C
        {"waveshape", 7.0f}, // Bell
        {"bpm", 80.0f},
        {"clockDiv", 3.0f}, // 1/8
        {"swing", 50.0f},
        {"attack", 0.005f}, // Instant attack (bell strike)
        {"hold", 0.1f},
        {"decay", 2.5f}, // Long ring
        {"sustain", 0.0f},
        {"release", 4.0f},
        {"filterCutoff", 12000.0f}, // Bright
        {"filterRes", 0.15f},
        {"filterMode", 0.0f},
        {"noiseLevel", 0.0f},
        {"subLevel", 0.0f},
        {"subOctave", 0.0f},
        {"masterVolume", 0.55f},
        {"voiceCount", 8.0f},
        {"melodicInertia", 0.45f},
        {"roundRobin", 0.35f},
        {"strumSpread", 8.0f}, // Slight arpeggio spread
        {"velocityHumanize", 0.12f},
        {"droneSustain", 0.0f},
        {"noteProbability", 0.45f},
        {"gateTime", 0.5f},
        {"consonance", 0.8f},   // Very consonant
        {"pitchGravity", 0.5f}, // Favor chord tones
        {"restProbability", 0.2f},
        {"maxTriggersPerStep", 4.0f},
        {"tuning", 0.0f},
        {"refPitch", 440.0f},
        {"stereoWidth", 0.7f},
        // FX: Shimmer + Reverb (crystalline space)
        {"shimmerOn", 1.0f},
        {"shimmerDecay", 0.75f},
        {"shimmerAmount", 0.35f},
        {"shimmerMix", 0.3f},
        {"reverbOn", 1.0f},
        {"reverbDecay", 0.7f},
        {"reverbDamping", 0.3f},
        {"reverbMix", 0.35f},
        // Off
        {"chorusOn", 0.0f},
        {"delayOn", 0.0f},
        {"phaserOn", 0.0f},
        {"flangerOn", 0.0f},
        {"bitcrushOn", 0.0f},
        {"tapeOn", 0.0f},
        {"pingPongOn", 0.0f}}},

      // ================================================================
      // 3: Dark Drone — deep sub, tape saturation, cavernous reverb
      // ================================================================
      {"Dark Drone",
       "Musical",
       {{"algorithm", 0.0f}, // GoL Classic (slow evolution)
        {"scale", 2.0f},     // Minor
        {"key", 2.0f},       // D
        {"waveshape", 2.0f}, // Saw
        {"bpm", 60.0f},
        {"clockDiv", 0.0f}, // 1/1 (very slow)
        {"swing", 50.0f},
        {"attack", 3.0f},
        {"hold", 1.0f},
        {"decay", 5.0f},
        {"sustain", 0.8f},
        {"release", 8.0f},
        {"filterCutoff", 800.0f}, // Dark, closed
        {"filterRes", 0.65f},
        {"filterMode", 0.0f},
        {"noiseLevel", 0.04f},
        {"subLevel", 0.8f},
        {"subOctave", 1.0f}, // -2 Oct
        {"masterVolume", 0.5f},
        {"voiceCount", 6.0f},
        {"melodicInertia", 0.9f}, // Heavy repetition (droning)
        {"roundRobin", 0.1f},
        {"strumSpread", 0.0f},
        {"velocityHumanize", 0.0f},
        {"droneSustain", 0.8f},
        {"noteProbability", 0.3f},
        {"gateTime", 1.0f},
        {"consonance", 0.6f},
        {"pitchGravity", 0.6f},    // Root-heavy
        {"restProbability", 0.1f}, // Relentless
        {"maxTriggersPerStep", 2.0f},
        {"tuning", 0.0f},
        {"refPitch", 440.0f},
        {"stereoWidth", 0.3f},
        // FX: Tape + Reverb + Delay (dark, cavernous)
        {"tapeOn", 1.0f},
        {"tapeDrive", 0.5f}, // Warm saturation
        {"tapeTone", 0.4f},  // Dark tone
        {"tapeMix", 0.35f},
        {"reverbOn", 1.0f},
        {"reverbDecay", 0.85f},  // Massive tail
        {"reverbDamping", 0.7f}, // Absorptive space
        {"reverbMix", 0.45f},
        {"delayOn", 1.0f},
        {"delayTime", 0.5f},
        {"delayFeedback", 0.3f},
        {"delayMix", 0.15f},
        // Off
        {"chorusOn", 0.0f},
        {"phaserOn", 0.0f},
        {"flangerOn", 0.0f},
        {"bitcrushOn", 0.0f},
        {"shimmerOn", 0.0f},
        {"pingPongOn", 0.0f}}},

      // ================================================================
      // 4: Pulsing Seeds — fast percussive, ping pong scatter
      // ================================================================
      {"Pulsing Seeds",
       "Experimental",
       {{"algorithm", 0.0f}, // GoL (high energy)
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
        {"masterVolume", 0.5f},
        {"voiceCount", 4.0f},
        {"melodicInertia", 0.0f},
        {"roundRobin", 0.8f},
        {"strumSpread", 0.0f},
        {"velocityHumanize", 0.2f},
        {"droneSustain", 0.0f},
        {"noteProbability", 0.4f},
        {"gateTime", 0.3f},
        {"consonance", 0.2f},   // Allow dissonance
        {"pitchGravity", 0.0f}, // Free pitch
        {"restProbability", 0.15f},
        {"maxTriggersPerStep", 3.0f},
        {"tuning", 0.0f},
        {"refPitch", 440.0f},
        {"stereoWidth", 0.9f}, // Wide scatter
        // FX: Ping Pong + Bitcrush (rhythmic chaos)
        {"pingPongOn", 1.0f},
        {"pingPongTime", 0.214f}, // Synced-ish to 140bpm
        {"pingPongFeedback", 0.5f},
        {"pingPongMix", 0.35f},
        {"bitcrushOn", 1.0f},
        {"bitcrushBits", 10.0f}, // Subtle grit
        {"bitcrushRate", 4.0f},
        {"bitcrushMix", 0.2f},
        // Off
        {"chorusOn", 0.0f},
        {"delayOn", 0.0f},
        {"reverbOn", 0.0f},
        {"phaserOn", 0.0f},
        {"flangerOn", 0.0f},
        {"tapeOn", 0.0f},
        {"shimmerOn", 0.0f}}},

      // ================================================================
      // 5: Ethereal Fifths — open tuning, chorus shimmer
      // ================================================================
      {"Ethereal Fifths",
       "Musical",
       {{"algorithm", 0.0f}, // GoL
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
        {"filterRes", 0.25f},
        {"filterMode", 0.0f},
        {"noiseLevel", 0.02f},
        {"subLevel", 0.3f},
        {"subOctave", 0.0f},
        {"masterVolume", 0.55f},
        {"voiceCount", 6.0f},
        {"melodicInertia", 0.7f},
        {"roundRobin", 0.4f},
        {"strumSpread", 15.0f},
        {"velocityHumanize", 0.1f},
        {"droneSustain", 0.0f},
        {"noteProbability", 0.5f},
        {"gateTime", 0.7f},
        {"consonance", 0.85f}, // Very consonant
        {"pitchGravity", 0.5f},
        {"restProbability", 0.2f},
        {"maxTriggersPerStep", 3.0f},
        {"tuning", 1.0f}, // Just Intonation
        {"refPitch", 440.0f},
        {"stereoWidth", 0.8f},
        // FX: Chorus + Shimmer (ethereal float)
        {"chorusOn", 1.0f},
        {"chorusRate", 0.25f}, // Slow, dreamy
        {"chorusDepth", 0.4f},
        {"chorusMix", 0.25f},
        {"shimmerOn", 1.0f},
        {"shimmerDecay", 0.7f},
        {"shimmerAmount", 0.3f},
        {"shimmerMix", 0.25f},
        {"reverbOn", 1.0f},
        {"reverbDecay", 0.7f},
        {"reverbDamping", 0.35f},
        {"reverbMix", 0.3f},
        // Off
        {"delayOn", 0.0f},
        {"phaserOn", 0.0f},
        {"flangerOn", 0.0f},
        {"bitcrushOn", 0.0f},
        {"tapeOn", 0.0f},
        {"pingPongOn", 0.0f}}},

      // ================================================================
      // 6: Nebula Drift — slow pad, phaser motion, deep reverb
      // ================================================================
      {"Nebula Drift",
       "Musical",
       {{"algorithm", 0.0f}, // GoL Classic
        {"scale", 9.0f},     // Pent. Major
        {"key", 0.0f},       // C
        {"waveshape", 6.0f}, // Pad
        {"bpm", 60.0f},
        {"clockDiv", 1.0f}, // 1/2
        {"swing", 50.0f},
        {"attack", 2.5f},
        {"hold", 1.0f},
        {"decay", 4.0f},
        {"sustain", 0.6f},
        {"release", 10.0f},
        {"filterCutoff", 2500.0f},
        {"filterRes", 0.35f},
        {"filterMode", 0.0f},
        {"noiseLevel", 0.02f},
        {"subLevel", 0.4f},
        {"subOctave", 0.0f},
        {"masterVolume", 0.5f},
        {"voiceCount", 5.0f},
        {"melodicInertia", 0.85f},
        {"roundRobin", 0.3f},
        {"strumSpread", 20.0f},
        {"velocityHumanize", 0.1f},
        {"droneSustain", 0.7f},
        {"noteProbability", 0.35f},
        {"gateTime", 1.0f},
        {"consonance", 0.75f},
        {"pitchGravity", 0.45f},
        {"restProbability", 0.25f},
        {"maxTriggersPerStep", 2.0f},
        {"tuning", 0.0f},
        {"refPitch", 440.0f},
        {"symmetry", 1.0f}, // 4-fold mirror
        {"stereoWidth", 0.6f},
        // FX: Phaser + Reverb (slow drifting motion)
        {"phaserOn", 1.0f},
        {"phaserRate", 0.15f}, // Very slow sweep
        {"phaserDepth", 0.55f},
        {"phaserMix", 0.3f},
        {"reverbOn", 1.0f},
        {"reverbDecay", 0.8f},
        {"reverbDamping", 0.4f},
        {"reverbMix", 0.45f},
        // Off
        {"chorusOn", 0.0f},
        {"delayOn", 0.0f},
        {"flangerOn", 0.0f},
        {"bitcrushOn", 0.0f},
        {"tapeOn", 0.0f},
        {"shimmerOn", 0.0f},
        {"pingPongOn", 0.0f}}},

      // ================================================================
      // 7: Tidal Lenia — organic waves, chorus + delay wash
      // ================================================================
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
        {"filterMode", 0.0f},
        {"noiseLevel", 0.02f},
        {"subLevel", 0.5f},
        {"subOctave", 0.0f},
        {"masterVolume", 0.5f},
        {"voiceCount", 5.0f},
        {"melodicInertia", 0.8f},
        {"roundRobin", 0.2f},
        {"strumSpread", 10.0f},
        {"velocityHumanize", 0.15f},
        {"droneSustain", 0.5f},
        {"noteProbability", 0.35f},
        {"gateTime", 0.9f},
        {"consonance", 0.7f},
        {"pitchGravity", 0.4f},
        {"restProbability", 0.2f},
        {"maxTriggersPerStep", 3.0f},
        {"tuning", 0.0f},
        {"refPitch", 440.0f},
        {"gridSize", 2.0f}, // Large for Lenia
        {"stereoWidth", 0.5f},
        // FX: Chorus + Delay + Reverb (oceanic wash)
        {"chorusOn", 1.0f},
        {"chorusRate", 0.2f},
        {"chorusDepth", 0.3f},
        {"chorusMix", 0.2f},
        {"delayOn", 1.0f},
        {"delayTime", 0.42f}, // Relaxed echo
        {"delayFeedback", 0.35f},
        {"delayMix", 0.2f},
        {"reverbOn", 1.0f},
        {"reverbDecay", 0.7f},
        {"reverbDamping", 0.45f},
        {"reverbMix", 0.35f},
        // Off
        {"phaserOn", 0.0f},
        {"flangerOn", 0.0f},
        {"bitcrushOn", 0.0f},
        {"tapeOn", 0.0f},
        {"shimmerOn", 0.0f},
        {"pingPongOn", 0.0f}}},

      // ================================================================
      // 8: Chemical Garden — RD textures, flanger + tape
      // ================================================================
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
        {"filterMode", 0.0f},
        {"noiseLevel", 0.04f},
        {"subLevel", 0.3f},
        {"subOctave", 0.0f},
        {"masterVolume", 0.5f},
        {"voiceCount", 3.0f},
        {"melodicInertia", 0.6f},
        {"roundRobin", 0.5f},
        {"strumSpread", 8.0f},
        {"velocityHumanize", 0.12f},
        {"droneSustain", 0.3f},
        {"noteProbability", 0.4f},
        {"gateTime", 0.7f},
        {"consonance", 0.4f}, // Allow some tension
        {"pitchGravity", 0.2f},
        {"restProbability", 0.2f},
        {"maxTriggersPerStep", 3.0f},
        {"tuning", 0.0f},
        {"refPitch", 440.0f},
        {"gridSize", 2.0f}, // Large for R-D
        {"stereoWidth", 0.6f},
        // FX: Flanger + Tape (chemical, metallic)
        {"flangerOn", 1.0f},
        {"flangerRate", 0.2f}, // Slow metallic sweep
        {"flangerDepth", 0.5f},
        {"flangerMix", 0.25f},
        {"tapeOn", 1.0f},
        {"tapeDrive", 0.4f},
        {"tapeTone", 0.55f},
        {"tapeMix", 0.25f},
        {"reverbOn", 1.0f},
        {"reverbDecay", 0.55f},
        {"reverbDamping", 0.5f},
        {"reverbMix", 0.2f},
        // Off
        {"chorusOn", 0.0f},
        {"delayOn", 0.0f},
        {"phaserOn", 0.0f},
        {"bitcrushOn", 0.0f},
        {"shimmerOn", 0.0f},
        {"pingPongOn", 0.0f}}},

      // ================================================================
      // 9: Neural Flicker — Brian's Brain, bitcrush + ping pong
      // ================================================================
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
        {"filterMode", 0.0f},
        {"noiseLevel", 0.08f},
        {"subLevel", 0.0f},
        {"subOctave", 0.0f},
        {"masterVolume", 0.45f},
        {"voiceCount", 5.0f},
        {"melodicInertia", 0.2f},
        {"roundRobin", 0.7f},
        {"strumSpread", 3.0f},
        {"velocityHumanize", 0.18f},
        {"droneSustain", 0.0f},
        {"noteProbability", 0.45f},
        {"gateTime", 0.25f},
        {"consonance", 0.15f}, // Chaotic, dissonant OK
        {"pitchGravity", 0.0f},
        {"restProbability", 0.1f},
        {"maxTriggersPerStep", 4.0f},
        {"tuning", 0.0f},
        {"refPitch", 440.0f},
        {"stereoWidth", 0.85f},
        // FX: Bitcrush + Ping Pong (digital glitch)
        {"bitcrushOn", 1.0f},
        {"bitcrushBits", 8.0f}, // Crunchy
        {"bitcrushRate", 6.0f}, // Sample rate reduction
        {"bitcrushMix", 0.3f},
        {"pingPongOn", 1.0f},
        {"pingPongTime", 0.185f}, // Fast bounce
        {"pingPongFeedback", 0.55f},
        {"pingPongMix", 0.3f},
        // Off
        {"chorusOn", 0.0f},
        {"delayOn", 0.0f},
        {"reverbOn", 0.0f},
        {"phaserOn", 0.0f},
        {"flangerOn", 0.0f},
        {"tapeOn", 0.0f},
        {"shimmerOn", 0.0f}}},

      // ================================================================
      // 10: Spectrum Cycle — Cyclic CA, phaser color wheel
      // ================================================================
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
        {"filterMode", 0.0f},
        {"noiseLevel", 0.0f},
        {"subLevel", 0.2f},
        {"subOctave", 0.0f},
        {"masterVolume", 0.5f},
        {"voiceCount", 4.0f},
        {"melodicInertia", 0.4f},
        {"roundRobin", 0.5f},
        {"strumSpread", 12.0f},
        {"velocityHumanize", 0.08f},
        {"droneSustain", 0.2f},
        {"noteProbability", 0.5f},
        {"gateTime", 0.6f},
        {"consonance", 0.3f}, // Whole tone = inherently ambiguous
        {"pitchGravity", 0.1f},
        {"restProbability", 0.2f},
        {"maxTriggersPerStep", 3.0f},
        {"tuning", 0.0f},
        {"refPitch", 440.0f},
        {"gridSize", 1.0f}, // Medium
        {"stereoWidth", 0.7f},
        // FX: Phaser + Chorus (swirling color)
        {"phaserOn", 1.0f},
        {"phaserRate", 0.35f}, // Medium sweep
        {"phaserDepth", 0.65f},
        {"phaserMix", 0.35f},
        {"chorusOn", 1.0f},
        {"chorusRate", 0.5f},
        {"chorusDepth", 0.35f},
        {"chorusMix", 0.2f},
        // Off
        {"delayOn", 0.0f},
        {"reverbOn", 0.0f},
        {"flangerOn", 0.0f},
        {"bitcrushOn", 0.0f},
        {"tapeOn", 0.0f},
        {"shimmerOn", 0.0f},
        {"pingPongOn", 0.0f}}},

      // ================================================================
      // 11: Swarm Murmuration — particle trails, delay + tape warmth
      // ================================================================
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
        {"filterRes", 0.45f},
        {"filterMode", 0.0f},
        {"noiseLevel", 0.03f},
        {"subLevel", 0.4f},
        {"subOctave", 0.0f},
        {"masterVolume", 0.5f},
        {"voiceCount", 3.0f},
        {"melodicInertia", 0.7f},
        {"roundRobin", 0.3f},
        {"strumSpread", 18.0f},
        {"velocityHumanize", 0.12f},
        {"droneSustain", 0.4f},
        {"noteProbability", 0.4f},
        {"gateTime", 0.8f},
        {"consonance", 0.65f},
        {"pitchGravity", 0.35f},
        {"restProbability", 0.2f},
        {"maxTriggersPerStep", 3.0f},
        {"tuning", 1.0f}, // Just Intonation
        {"refPitch", 440.0f},
        {"gridSize", 2.0f}, // Large
        {"stereoWidth", 0.7f},
        // FX: Delay + Tape + Reverb (warm, trailing)
        {"delayOn", 1.0f},
        {"delayTime", 0.36f}, // 1/4 note at 84bpm
        {"delayFeedback", 0.4f},
        {"delayMix", 0.25f},
        {"tapeOn", 1.0f},
        {"tapeDrive", 0.3f},
        {"tapeTone", 0.65f},
        {"tapeMix", 0.2f},
        {"reverbOn", 1.0f},
        {"reverbDecay", 0.6f},
        {"reverbDamping", 0.5f},
        {"reverbMix", 0.25f},
        // Off
        {"chorusOn", 0.0f},
        {"phaserOn", 0.0f},
        {"flangerOn", 0.0f},
        {"bitcrushOn", 0.0f},
        {"shimmerOn", 0.0f},
        {"pingPongOn", 0.0f}}},

      // ================================================================
      // 12: Fog Machine — Brownian drones, deep reverb + chorus
      // ================================================================
      {"Fog Machine",
       "Musical",
       {{"algorithm", 7.0f}, // Brownian Field
        {"scale", 9.0f},     // Pent. Major
        {"key", 5.0f},       // F
        {"waveshape", 6.0f}, // Pad
        {"bpm", 55.0f},
        {"clockDiv", 0.0f}, // 1/1
        {"swing", 50.0f},
        {"attack", 4.0f},
        {"hold", 1.0f},
        {"decay", 6.0f},
        {"sustain", 0.7f},
        {"release", 10.0f},
        {"filterCutoff", 1800.0f},
        {"filterRes", 0.35f},
        {"filterMode", 0.0f},
        {"noiseLevel", 0.06f},
        {"subLevel", 0.6f},
        {"subOctave", 1.0f}, // -2 Oct
        {"masterVolume", 0.45f},
        {"voiceCount", 4.0f},
        {"melodicInertia", 0.95f},
        {"roundRobin", 0.1f},
        {"strumSpread", 0.0f},
        {"velocityHumanize", 0.05f},
        {"droneSustain", 0.9f},
        {"noteProbability", 0.25f},
        {"gateTime", 1.0f},
        {"consonance", 0.8f}, // Consonant drones
        {"pitchGravity", 0.6f},
        {"restProbability", 0.3f},
        {"maxTriggersPerStep", 2.0f},
        {"tuning", 2.0f}, // Pythagorean
        {"refPitch", 440.0f},
        {"gridSize", 2.0f},
        {"stereoWidth", 0.3f},
        // FX: Reverb + Chorus + Delay (foggy, diffuse)
        {"reverbOn", 1.0f},
        {"reverbDecay", 0.85f},
        {"reverbDamping", 0.5f},
        {"reverbMix", 0.55f},
        {"chorusOn", 1.0f},
        {"chorusRate", 0.12f}, // Very slow
        {"chorusDepth", 0.35f},
        {"chorusMix", 0.2f},
        {"delayOn", 1.0f},
        {"delayTime", 0.8f},
        {"delayFeedback", 0.2f},
        {"delayMix", 0.15f},
        // Off
        {"phaserOn", 0.0f},
        {"flangerOn", 0.0f},
        {"bitcrushOn", 0.0f},
        {"tapeOn", 0.0f},
        {"shimmerOn", 0.0f},
        {"pingPongOn", 0.0f}}},

      // ================================================================
      // 13: Cathedral Organ — massive shimmer reverb, tape warmth
      // ================================================================
      {"Cathedral Organ",
       "Musical",
       {{"algorithm", 0.0f}, // GoL Classic
        {"scale", 1.0f},     // Major
        {"key", 0.0f},       // C
        {"waveshape", 6.0f}, // Pad
        {"bpm", 50.0f},
        {"clockDiv", 0.0f}, // 1/1 (majestic)
        {"swing", 50.0f},
        {"attack", 3.5f},
        {"hold", 2.0f},
        {"decay", 5.0f},
        {"sustain", 0.85f},
        {"release", 12.0f},
        {"filterCutoff", 2200.0f},
        {"filterRes", 0.2f},
        {"filterMode", 0.0f},
        {"noiseLevel", 0.01f},
        {"subLevel", 0.7f},
        {"subOctave", 1.0f}, // -2 Oct (organ pedals)
        {"masterVolume", 0.45f},
        {"voiceCount", 8.0f},
        {"melodicInertia", 0.9f},
        {"roundRobin", 0.1f},
        {"strumSpread", 0.0f},
        {"velocityHumanize", 0.02f},
        {"droneSustain", 0.85f},
        {"noteProbability", 0.3f},
        {"gateTime", 1.0f},
        {"consonance", 0.9f},   // Very consonant (organ music)
        {"pitchGravity", 0.7f}, // Strong chord tones
        {"restProbability", 0.15f},
        {"maxTriggersPerStep", 2.0f},
        {"tuning", 1.0f}, // Just Intonation
        {"refPitch", 440.0f},
        {"symmetry", 2.0f}, // 8-fold mirror
        {"gridSize", 3.0f}, // XL grid
        {"stereoWidth", 0.65f},
        // FX: Shimmer + Reverb + Tape (organ through a cathedral)
        {"shimmerOn", 1.0f},
        {"shimmerDecay", 0.8f},
        {"shimmerAmount", 0.35f},
        {"shimmerMix", 0.3f},
        {"reverbOn", 1.0f},
        {"reverbDecay", 0.85f},
        {"reverbDamping", 0.25f}, // Bright reflections
        {"reverbMix", 0.55f},
        {"tapeOn", 1.0f},
        {"tapeDrive", 0.2f}, // Subtle warmth
        {"tapeTone", 0.7f},
        {"tapeMix", 0.15f},
        // Off
        {"chorusOn", 0.0f},
        {"delayOn", 0.0f},
        {"phaserOn", 0.0f},
        {"flangerOn", 0.0f},
        {"bitcrushOn", 0.0f},
        {"pingPongOn", 0.0f}}},

      // ================================================================
      // 14: Storm Front — chaotic particles, ping pong + flanger
      // ================================================================
      {"Storm Front",
       "Experimental",
       {{"algorithm", 5.0f}, // Particle Swarm
        {"scale", 0.0f},     // Chromatic
        {"key", 2.0f},       // D
        {"waveshape", 2.0f}, // Saw
        {"bpm", 135.0f},
        {"clockDiv", 3.0f}, // 1/8
        {"swing", 65.0f},
        {"attack", 0.01f},
        {"hold", 0.0f},
        {"decay", 0.4f},
        {"sustain", 0.1f},
        {"release", 1.5f},
        {"filterCutoff", 7000.0f},
        {"filterRes", 0.55f},
        {"filterMode", 0.0f},
        {"noiseLevel", 0.12f},
        {"subLevel", 0.2f},
        {"subOctave", 0.0f},
        {"masterVolume", 0.4f},
        {"voiceCount", 12.0f},
        {"melodicInertia", 0.1f},
        {"roundRobin", 0.9f},
        {"strumSpread", 2.0f},
        {"velocityHumanize", 0.25f},
        {"droneSustain", 0.0f},
        {"noteProbability", 0.55f},
        {"gateTime", 0.35f},
        {"consonance", 0.1f}, // Maximum chaos
        {"pitchGravity", 0.0f},
        {"restProbability", 0.05f},
        {"maxTriggersPerStep", 6.0f},
        {"tuning", 0.0f},
        {"refPitch", 440.0f},
        {"gridSize", 4.0f}, // XXL grid
        {"stereoWidth", 1.0f},
        // FX: Ping Pong + Flanger + Chorus (chaotic storm)
        {"pingPongOn", 1.0f},
        {"pingPongTime", 0.22f},
        {"pingPongFeedback", 0.6f},
        {"pingPongMix", 0.35f},
        {"flangerOn", 1.0f},
        {"flangerRate", 1.5f}, // Fast metallic sweep
        {"flangerDepth", 0.7f},
        {"flangerMix", 0.3f},
        {"chorusOn", 1.0f},
        {"chorusRate", 2.5f},
        {"chorusDepth", 0.6f},
        {"chorusMix", 0.25f},
        // Off
        {"delayOn", 0.0f},
        {"reverbOn", 0.0f},
        {"phaserOn", 0.0f},
        {"bitcrushOn", 0.0f},
        {"tapeOn", 0.0f},
        {"shimmerOn", 0.0f}}},

      // ================================================================
      // 15: Deep Ocean — Lenia, shimmer + reverb + chorus (immersive)
      // ================================================================
      {"Deep Ocean",
       "Musical",
       {{"algorithm", 6.0f}, // Lenia
        {"scale", 9.0f},     // Pent. Major
        {"key", 5.0f},       // F
        {"waveshape", 0.0f}, // Sine (pure, oceanic)
        {"bpm", 45.0f},
        {"clockDiv", 0.0f}, // 1/1
        {"swing", 50.0f},
        {"attack", 5.0f},
        {"hold", 2.0f},
        {"decay", 8.0f},
        {"sustain", 0.6f},
        {"release", 15.0f},
        {"filterCutoff", 1500.0f},
        {"filterRes", 0.25f},
        {"filterMode", 0.0f},
        {"noiseLevel", 0.06f}, // Ocean spray
        {"subLevel", 0.6f},
        {"subOctave", 1.0f}, // -2 Oct (deep rumble)
        {"masterVolume", 0.4f},
        {"voiceCount", 10.0f},
        {"melodicInertia", 0.95f},
        {"roundRobin", 0.1f},
        {"strumSpread", 30.0f},
        {"velocityHumanize", 0.1f},
        {"droneSustain", 0.85f},
        {"noteProbability", 0.2f},
        {"gateTime", 1.0f},
        {"consonance", 0.85f},
        {"pitchGravity", 0.55f},
        {"restProbability", 0.3f},
        {"maxTriggersPerStep", 2.0f},
        {"tuning", 1.0f}, // Just Intonation
        {"refPitch", 440.0f},
        {"gridSize", 7.0f}, // Huge grid
        {"symmetry", 1.0f}, // 4-fold mirror
        {"stereoWidth", 0.8f},
        // FX: Shimmer + Reverb + Chorus (deep, immersive)
        {"shimmerOn", 1.0f},
        {"shimmerDecay", 0.8f},
        {"shimmerAmount", 0.3f},
        {"shimmerMix", 0.25f},
        {"reverbOn", 1.0f},
        {"reverbDecay", 0.85f},
        {"reverbDamping", 0.4f},
        {"reverbMix", 0.5f},
        {"chorusOn", 1.0f},
        {"chorusRate", 0.12f}, // Slow shimmer
        {"chorusDepth", 0.35f},
        {"chorusMix", 0.2f},
        // Off
        {"delayOn", 0.0f},
        {"phaserOn", 0.0f},
        {"flangerOn", 0.0f},
        {"bitcrushOn", 0.0f},
        {"tapeOn", 0.0f},
        {"pingPongOn", 0.0f}}},
  };
}
