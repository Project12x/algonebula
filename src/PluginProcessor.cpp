#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AlgoNebulaProcessor::AlgoNebulaProcessor()
    : AudioProcessor(BusesProperties().withOutput(
          "Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "AlgoNebulaState", createParameterLayout()),
      engine(createEngine(0)) {}

std::unique_ptr<CellularEngine>
AlgoNebulaProcessor::createEngine(int algoIdx, int rows, int cols) {
  switch (algoIdx) {
  case 0:
    return std::make_unique<GameOfLife>(rows, cols,
                                        GameOfLife::RulePreset::Classic);
  case 1:
    return std::make_unique<GameOfLife>(rows, cols,
                                        GameOfLife::RulePreset::HighLife);
  case 2:
    return std::make_unique<BriansBrain>(rows, cols);
  case 3:
    return std::make_unique<CyclicCA>(rows, cols);
  case 4:
    return std::make_unique<ReactionDiffusion>(rows, cols);
  case 5:
    return std::make_unique<ParticleSwarm>(rows, cols);
  case 6:
    return std::make_unique<LeniaEngine>(rows, cols);
  case 7:
    return std::make_unique<BrownianField>(rows, cols);
  default:
    return std::make_unique<GameOfLife>(rows, cols,
                                        GameOfLife::RulePreset::Classic);
  }
}

AlgoNebulaProcessor::~AlgoNebulaProcessor() = default;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
AlgoNebulaProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  // --- Master ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("masterVolume", 1), "Master Volume",
      juce::NormalisableRange<float>(0.0f, 2.0f, 0.001f), 0.5f));

  // --- Algorithm ---
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("algorithm", 1), "Algorithm",
      juce::StringArray{"Game of Life", "Wolfram 1D", "Brian's Brain",
                        "Cyclic CA", "Reaction-Diffusion", "Particle Swarm",
                        "Lenia", "Brownian Field"},
      0));

  // --- Clock ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("bpm", 1), "BPM",
      juce::NormalisableRange<float>(40.0f, 300.0f, 0.1f), 120.0f));

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("clockDiv", 1), "Clock Division",
      juce::StringArray{"1/1", "1/2", "1/4", "1/8", "1/16", "1/32"}, 2));

  // --- Scale ---
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("scale", 1), "Scale",
      juce::StringArray{"Chromatic", "Major", "Minor", "Dorian", "Phrygian",
                        "Lydian", "Mixolydian", "Aeolian", "Locrian",
                        "Pent. Major", "Pent. Minor", "Blues", "Whole Tone",
                        "Harmonic Minor", "Melodic Minor"},
      1)); // Major

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("key", 1), "Key",
      juce::StringArray{"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A",
                        "A#", "B"},
      0));

  // --- Voices ---
  layout.add(std::make_unique<juce::AudioParameterInt>(
      juce::ParameterID("voiceCount", 1), "Voice Count", 1, 64, 3));

  // --- Waveshape ---
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("waveshape", 1), "Waveshape",
      juce::StringArray{"Sine", "Triangle", "Saw", "Pulse", "Sine+Oct",
                        "Fifth Stack", "Pad", "Bell"},
      0));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("waveshapeSpread", 1), "Waveshape Spread",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  // --- Ambient ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("droneSustain", 1), "Drone Sustain",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("noteProbability", 1), "Note Probability",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("gateTime", 1), "Gate Time",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

  // --- Humanization ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("swing", 1), "Swing",
      juce::NormalisableRange<float>(50.0f, 75.0f, 0.1f), 50.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("strumSpread", 1), "Strum Spread",
      juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("melodicInertia", 1), "Melodic Inertia",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("roundRobin", 1), "Round Robin",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.2f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("velocityHumanize", 1), "Velocity Humanize",
      juce::NormalisableRange<float>(0.0f, 0.5f, 0.01f), 0.1f));

  // --- Envelope ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("attack", 1), "Attack",
      juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.8f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("hold", 1), "Hold",
      juce::NormalisableRange<float>(0.0f, 2.0f, 0.001f, 0.5f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("decay", 1), "Decay",
      juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.5f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("sustain", 1), "Sustain",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("release", 1), "Release",
      juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 3.0f));

  // --- Filter ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("filterCutoff", 1), "Filter Cutoff",
      juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 8000.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("filterRes", 1), "Filter Resonance",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("filterMode", 1), "Filter Mode",
      juce::StringArray{"Low Pass", "High Pass", "Band Pass", "Notch"}, 0));

  // --- Noise + Sub ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("noiseLevel", 1), "Noise Level",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("subLevel", 1), "Sub Level",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("subOctave", 1), "Sub Octave",
      juce::StringArray{"-1 Oct", "-2 Oct"}, 0));

  // --- Tuning ---
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("tuning", 1), "Tuning",
      juce::StringArray{"12-TET", "Just Intonation", "Pythagorean"}, 0));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("refPitch", 1), "Reference Pitch",
      juce::NormalisableRange<float>(420.0f, 460.0f, 0.1f), 440.0f));

  // --- Symmetry ---
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("symmetry", 1), "Symmetry",
      juce::StringArray{"None", "4-Fold Mirror"}, 1)); // default to symmetric

  // --- Grid Resolution ---
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("gridSize", 1), "Grid Size",
      juce::StringArray{"Small (8x12)", "Medium (12x16)", "Large (16x24)",
                        "XL (24x32)", "XXL (32x48)", "Epic (48x64)",
                        "Massive (64x96)", "Huge (128x128)",
                        "Experimental (256x256)", "Extreme (512x512)",
                        "Insane (1024x1024)", "Ultra (1280x1280)"},
      1)); // default to Medium

  // --- Freeze ---
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("freeze", 1), "Freeze", juce::StringArray{"Off", "On"},
      0));

  // --- Anti-cacophony ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("consonance", 1), "Consonance",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

  layout.add(std::make_unique<juce::AudioParameterInt>(
      juce::ParameterID("maxTriggersPerStep", 1), "Max Triggers/Step", 1, 8,
      3));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("restProbability", 1), "Rest Probability",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.2f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("pitchGravity", 1), "Pitch Gravity",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

  // --- Stereo Width ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("stereoWidth", 1), "Stereo Width",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

  // --- Chorus ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("chorusRate", 1), "Chorus Rate",
      juce::NormalisableRange<float>(0.1f, 5.0f, 0.01f), 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("chorusDepth", 1), "Chorus Depth",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.4f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("chorusMix", 1), "Chorus Mix",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  // --- Delay ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("delayTime", 1), "Delay Time",
      juce::NormalisableRange<float>(0.01f, 2.0f, 0.01f), 0.3f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("delayFeedback", 1), "Delay Feedback",
      juce::NormalisableRange<float>(0.0f, 0.75f, 0.01f), 0.4f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("delayMix", 1), "Delay Mix",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  // --- Reverb ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("reverbDecay", 1), "Reverb Decay",
      juce::NormalisableRange<float>(0.0f, 0.85f, 0.01f), 0.7f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("reverbDamping", 1), "Reverb Damping",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("reverbMix", 1), "Reverb Mix",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  // --- Phaser ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("phaserRate", 1), "Phaser Rate",
      juce::NormalisableRange<float>(0.05f, 5.0f, 0.01f), 0.4f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("phaserDepth", 1), "Phaser Depth",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("phaserMix", 1), "Phaser Mix",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  // --- Flanger ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("flangerRate", 1), "Flanger Rate",
      juce::NormalisableRange<float>(0.05f, 5.0f, 0.01f), 0.3f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("flangerDepth", 1), "Flanger Depth",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.6f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("flangerMix", 1), "Flanger Mix",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  // --- Bitcrush ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("bitcrushBits", 1), "Bitcrush Depth",
      juce::NormalisableRange<float>(1.0f, 16.0f, 0.1f), 8.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("bitcrushRate", 1), "Bitcrush Rate",
      juce::NormalisableRange<float>(1.0f, 50.0f, 0.1f), 1.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("bitcrushMix", 1), "Bitcrush Mix",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  // --- Tape Saturation ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("tapeDrive", 1), "Tape Drive",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("tapeTone", 1), "Tape Tone",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("tapeMix", 1), "Tape Sat Mix",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  // --- Shimmer Reverb ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("shimmerDecay", 1), "Shimmer Decay",
      juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f), 0.8f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("shimmerAmount", 1), "Shimmer Amount",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("shimmerMix", 1), "Shimmer Mix",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  // --- Ping Pong Delay ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("pingPongTime", 1), "Ping Pong Time",
      juce::NormalisableRange<float>(0.01f, 2.0f, 0.01f), 0.375f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("pingPongFeedback", 1), "Ping Pong Feedback",
      juce::NormalisableRange<float>(0.0f, 0.85f, 0.01f), 0.4f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("pingPongMix", 1), "Ping Pong Mix",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  // --- Effect On/Off Toggles ---
  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID("chorusOn", 1), "Chorus On", false));
  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID("delayOn", 1), "Delay On", false));
  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID("reverbOn", 1), "Reverb On", false));
  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID("phaserOn", 1), "Phaser On", false));
  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID("flangerOn", 1), "Flanger On", false));
  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID("bitcrushOn", 1), "Bitcrush On", false));
  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID("tapeOn", 1), "Tape Sat On", false));
  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID("shimmerOn", 1), "Shimmer On", false));
  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID("pingPongOn", 1), "Ping Pong On", false));

  // --- Trigger Budget (0 = use engine default) ---
  layout.add(std::make_unique<juce::AudioParameterInt>(
      juce::ParameterID("triggerBudget", 1), "Trigger Budget", 0, 32, 0));

  // --- Modulation LFO 1 ---
  layout.add(std::make_unique<juce::AudioParameterInt>(
      juce::ParameterID("lfo1Shape", 1), "LFO 1 Shape", 0, 4, 0));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("lfo1Rate", 1), "LFO 1 Rate",
      juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.4f), 1.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("lfo1Amount", 1), "LFO 1 Amount",
      juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      juce::ParameterID("lfo1Dest", 1), "LFO 1 Dest", 0, 17, 0));

  // --- Modulation LFO 2 ---
  layout.add(std::make_unique<juce::AudioParameterInt>(
      juce::ParameterID("lfo2Shape", 1), "LFO 2 Shape", 0, 4, 0));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("lfo2Rate", 1), "LFO 2 Rate",
      juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.4f), 1.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("lfo2Amount", 1), "LFO 2 Amount",
      juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));
  layout.add(std::make_unique<juce::AudioParameterInt>(
      juce::ParameterID("lfo2Dest", 1), "LFO 2 Dest", 0, 17, 0));

  // --- Musicality Hard Rules ---
  layout.add(std::make_unique<juce::AudioParameterInt>(
      juce::ParameterID("maxLeap", 1), "Max Leap", 0, 24, 12));

  layout.add(std::make_unique<juce::AudioParameterInt>(
      juce::ParameterID("baseOctave", 1), "Base Octave", 1, 6, 3));

  layout.add(std::make_unique<juce::AudioParameterInt>(
      juce::ParameterID("octaveRange", 1), "Octave Range", 1, 5, 3));

  // --- Musicality bypass ---
  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID("musicalityBypass", 1), "Musicality Bypass", false));

  // --- GPU Acceleration (opt-in, default OFF) ---
  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID("gpuAccel", 1), "GPU Acceleration", false));

  return layout;
}

//==============================================================================
void AlgoNebulaProcessor::prepareToPlay(double sampleRate,
                                        int samplesPerBlock) {
  currentSampleRate = sampleRate;
  currentBlockSize = samplesPerBlock;

  // Pre-allocate stereo mix buffer (no allocations on audio thread)
  stereoMixBuffer.setSize(2, samplesPerBlock, false, true, false);

  // Initialize smoothed parameters (20ms ramp time prevents zipper noise)
  masterVolume.reset(sampleRate, 0.02);
  masterVolume.setCurrentAndTargetValue(
      apvts.getRawParameterValue("masterVolume")->load());
  smoothFilterCutoff.reset(sampleRate, 0.02);
  smoothFilterCutoff.setCurrentAndTargetValue(
      apvts.getRawParameterValue("filterCutoff")->load());
  smoothFilterRes.reset(sampleRate, 0.02);
  smoothFilterRes.setCurrentAndTargetValue(
      apvts.getRawParameterValue("filterRes")->load());
  smoothNoiseLevel.reset(sampleRate, 0.02);
  smoothNoiseLevel.setCurrentAndTargetValue(
      apvts.getRawParameterValue("noiseLevel")->load());
  smoothSubLevel.reset(sampleRate, 0.02);
  smoothSubLevel.setCurrentAndTargetValue(
      apvts.getRawParameterValue("subLevel")->load());
  smoothDensityGain.reset(sampleRate, 0.05); // slower ramp for density
  smoothDensityGain.setCurrentAndTargetValue(1.0f);

  // Initialize DSP effects
  float sr = static_cast<float>(sampleRate);
  chorus.init(sr);
  delay.init(sr);
  reverb.init(sr);
  phaser.init(sr);
  flanger.init(sr);
  bitcrush.init(sr);
  tapeSat.init(sr);
  shimmer.init(sr);
  pingPong.init(sr);
  safetyProc.init(sr);
  modLfo1.init(sr);
  modLfo2.init(sr);

  // Set up effect chain slots (order: modulation -> delay -> reverb ->
  // distortion)
  effectChain.setSlot(0, &chorus);
  effectChain.setSlot(1, &phaser);
  effectChain.setSlot(2, &flanger);
  effectChain.setSlot(3, &delay);
  effectChain.setSlot(4, &pingPong);
  effectChain.setSlot(5, &reverb);
  effectChain.setSlot(6, &shimmer);
  effectChain.setSlot(7, &bitcrush);
  effectChain.setSlot(8, &tapeSat);
  effectChain.init(sr);

  // Initialize safety limiter (brick-wall, before SafetyProcessor)
  {
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;
    safetyLimiter.prepare(spec);
    safetyLimiter.setThreshold(-1.0f); // -1 dB ceiling
    safetyLimiter.setRelease(5.0f);    // 5ms release
  }

  // Initialize engine with default seed
  engine->randomize(42, 0.3f);
  gpuCompute.getBridge().updateFromCpu(engine->getGrid());
  {
    int backIdx = 1 - gridReadIdx_.load(std::memory_order_relaxed);
    gpuCompute.getBridge().convertToGrid(gridSnapshots_[backIdx]);
    gridReadIdx_.store(backIdx, std::memory_order_release);
  }
  engineGeneration.store(0, std::memory_order_relaxed);

  // Initialize CPU step timer (runs engine->step() on message thread)
  cpuStepTimer_.setTargets(engine.get(), &gpuCompute.getBridge(), &cellEditQueue);
  // Timer must be started from message thread (prepareToPlay is audio thread)
  juce::MessageManager::callAsync([this]() { cpuStepTimer_.start(); });

  // Initialize clock
  clock.reset(sampleRate);
  clock.setBPM(120.0);
  clock.setDivision(ClockDivider::Division::Quarter);

  // Pre-compute tuning table
  tuning.setSystem(Microtuning::System::TwelveTET, 440.0f);

  // Initialize voices
  for (int i = 0; i < kMaxVoices; ++i)
    voices[i].reset();
}

void AlgoNebulaProcessor::releaseResources() {
  // Nothing to release in Phase 1
}

void AlgoNebulaProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                       juce::MidiBuffer &midiMessages) {
  auto startTime = juce::Time::getHighResolutionTicks();

  juce::ScopedNoDenormals noDenormals;

  // Cell edits drained by CpuStepTimer on message thread

  // Process virtual MIDI keyboard input
  keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(),
                                      true);

  // MIDI note-on: key tracking and velocity (no grid reseed ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â preserves
  // symmetric CA growth patterns from manual seeds)
  for (const auto metadata : midiMessages) {
    const auto msg = metadata.getMessage();
    if (msg.isNoteOn()) {
      // Key tracking: set root key from MIDI note
      int noteKey = msg.getNoteNumber() % 12;
      if (auto *keyParam = apvts.getParameter("key")) {
        keyParam->setValueNotifyingHost(
            keyParam->convertTo0to1(static_cast<float>(noteKey)));
      }

      // Store velocity for voice triggering
      lastMidiVelocity = msg.getFloatVelocity();
    }
  }

  // Read smoothed parameters
  masterVolume.setTargetValue(
      apvts.getRawParameterValue("masterVolume")->load());
  smoothFilterCutoff.setTargetValue(
      apvts.getRawParameterValue("filterCutoff")->load());
  smoothFilterRes.setTargetValue(
      apvts.getRawParameterValue("filterRes")->load());
  smoothNoiseLevel.setTargetValue(
      apvts.getRawParameterValue("noiseLevel")->load());
  smoothSubLevel.setTargetValue(apvts.getRawParameterValue("subLevel")->load());

  const int numSamples = buffer.getNumSamples();

  // --- Read clock params and update clock ---
  float bpm = apvts.getRawParameterValue("bpm")->load();
  int clockDivIdx =
      static_cast<int>(apvts.getRawParameterValue("clockDiv")->load());
  float swing = apvts.getRawParameterValue("swing")->load();
  clock.setBPM(static_cast<double>(bpm));
  clock.setDivision(static_cast<ClockDivider::Division>(clockDivIdx));
  clock.setSwing(static_cast<double>(swing));

  // --- Read algorithm and grid size params, switch engine type ---
  int algoIdx =
      static_cast<int>(apvts.getRawParameterValue("algorithm")->load());
  int gridSizeIdx =
      static_cast<int>(apvts.getRawParameterValue("gridSize")->load());

  // Grid size lookup: rows, cols
  static constexpr int kGridSizes[][2] = {
      {8, 12},  {12, 16},   {16, 24},   {24, 32},   {32, 48},     {48, 64},
      {64, 96}, {128, 128}, {256, 256}, {512, 512}, {1024, 1024}, {1280, 1280}};
  int gridRows = kGridSizes[gridSizeIdx][0];
  int gridCols = kGridSizes[gridSizeIdx][1];

  // --- Read GPU acceleration toggle ---
  bool wantGpu = apvts.getRawParameterValue("gpuAccel")->load() > 0.5f;

  if (algoIdx != lastAlgorithmIdx || gridSizeIdx != lastGridSizeIdx) {
    lastAlgorithmIdx = algoIdx;
    lastGridSizeIdx = gridSizeIdx;
    // Release all voices when switching engine
    for (int v = 0; v < kMaxVoices; ++v)
      voices[v].reset();

    // GPU needs reinit with new engine -- stop current, toggle-on will reinit
    if (gpuActive.load(std::memory_order_relaxed)) {
      gpuActive.store(false, std::memory_order_relaxed);
      auto *mgr = &gpuCompute;
      juce::MessageManager::callAsync([mgr]() { mgr->stop(); });
    }

    // Defer engine recreation to message thread (timer uses engine pointer)
    cpuStepTimer_.stop();
    int capturedAlgo = algoIdx;
    int capturedRows = gridRows;
    int capturedCols = gridCols;
    uint64_t capturedSeed = reseedRng;
    juce::MessageManager::callAsync([this, capturedAlgo, capturedRows, capturedCols, capturedSeed]() {
      engine = createEngine(capturedAlgo, capturedRows, capturedCols);
      engine->randomize(capturedSeed, 0.3f);
      gpuCompute.getBridge().updateFromCpu(engine->getGrid());
      cpuStepTimer_.setTargets(engine.get(), &gpuCompute.getBridge(), &cellEditQueue);
      cpuStepTimer_.start();
    });
  }
  // Toggle GPU on/off ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â defer to message thread for timer/device safety
  if (wantGpu && !gpuActive.load(std::memory_order_relaxed) && !gpuPending.load(std::memory_order_relaxed)) {
    auto eType = engine->getType();
    int gRows = gridRows;
    int gCols = gridCols;
    auto *mgr = &gpuCompute;
    auto *flag = &gpuActive;
    gpuPending.store(true, std::memory_order_relaxed);
    auto *pending = &gpuPending;
    uint64_t seedVal = reseedRng;
    juce::MessageManager::callAsync([mgr, flag, pending, eType, gRows, gCols, seedVal]() {
      if (mgr->setEngine(eType, gRows, gCols)) {
        mgr->seed(seedVal, 0.3f);
        if (mgr->start()) {
          flag->store(true, std::memory_order_relaxed);
        }
      }
      pending->store(false, std::memory_order_relaxed);
    });
    reseedCooldown = 120; // let GPU warm up before checking stagnation
  } else if (!wantGpu && gpuActive.load(std::memory_order_relaxed)) {
    gpuActive.store(false, std::memory_order_relaxed);
    auto *mgr = &gpuCompute;
    juce::MessageManager::callAsync([mgr]() { mgr->stop(); });
  }

  // Clear output buffer
  for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    buffer.clear(ch, 0, numSamples);

  // --- Handle transport requests from UI thread ---
  bool isGpu = gpuActive.load(std::memory_order_relaxed);

  // Seed change from user input
  if (seedChanged.load(std::memory_order_relaxed)) {
    reseedRng = currentSeed.load(std::memory_order_relaxed);
    seedChanged.store(false, std::memory_order_relaxed);
    // In GPU mode, re-seed the GPU simulation
    if (isGpu) {
      auto *mgr = &gpuCompute;
      juce::MessageManager::callAsync([mgr, reseedRng = reseedRng]() {
        mgr->seed(reseedRng, 0.3f);
      });
    } else {
      cpuStepTimer_.requestReseed(reseedRng, 0.3f, false);
    }
    stagnationCounter = 0;
    lastAliveCount = 0;
    // Release all voices on seed change (graceful fade-out)
    for (int i = 0; i < kMaxVoices; ++i)
      voices[i].noteOff();
  }

  // Clear grid request
  if (clearRequested.exchange(false, std::memory_order_relaxed)) {
    if (isGpu) {
      auto *mgr = &gpuCompute;
      juce::MessageManager::callAsync([mgr]() { mgr->clearState(); });
    } else {
      cpuStepTimer_.requestClear();
    }
    // Kill all voices immediately on clear
    for (int i = 0; i < kMaxVoices; ++i)
      voices[i].reset();
    stagnationCounter = 0;
    lastAliveCount = 0;
  }

  // Reseed request (always symmetric)
  if (reseedSymmetricRequested.exchange(false, std::memory_order_relaxed)) {
    reseedRng ^= reseedRng << 13;
    reseedRng ^= reseedRng >> 7;
    reseedRng ^= reseedRng << 17;
    currentSeed.store(reseedRng, std::memory_order_relaxed);
    if (isGpu) {
      auto *mgr = &gpuCompute;
      juce::MessageManager::callAsync([mgr, reseedRng = reseedRng]() {
        mgr->seed(reseedRng, 0.3f);
      });
    } else {
      cpuStepTimer_.requestReseed(reseedRng, 0.3f, true);
    }
    stagnationCounter = 0;
    lastAliveCount = 0;
    // Release all voices on reseed (graceful fade-out)
    for (int i = 0; i < kMaxVoices; ++i)
      voices[i].noteOff();
  }

  // --- Read symmetry mode ---
  int symmetryIdx =
      static_cast<int>(apvts.getRawParameterValue("symmetry")->load());
  bool useSymmetry = (symmetryIdx == 1);

  // Load factory pattern request
  int patternIdx = loadPatternRequested.exchange(-1, std::memory_order_relaxed);
  if (patternIdx >= 0) {
    // Defer pattern load to message thread (engine safety)
    juce::MessageManager::callAsync([this, patternIdx]() {
      FactoryPatternLibrary::applyPattern(engine->getGridMutable(), patternIdx);
    });
    stagnationCounter = 0;
    lastAliveCount = 0;
  }

  // --- Read freeze mode ---
  bool isFrozen =
      static_cast<int>(apvts.getRawParameterValue("freeze")->load()) == 1;

  // Clock-driven engine stepping (only when running and not frozen)
  stepTriggeredThisBlock = false;
  bool isRunning = engineRunning.load(std::memory_order_relaxed);

  // Propagate frozen state to all voices (pauses gate timers)
  for (int v = 0; v < kMaxVoices; ++v)
    voices[v].setFrozen(isFrozen);


  // GPU path: simulation runs on GPU timer thread; CPU path: step on clock tick
  auto& bridge = gpuCompute.getBridge();
  if (gpuActive.load(std::memory_order_relaxed)) {
    // GPU is stepping via GpuComputeManager timer.
    // Bridge is updated by readback callback on message thread.
    for (int i = 0; i < numSamples; ++i) {
      if (clock.tick() && isRunning && !isFrozen)
        stepTriggeredThisBlock = true;
    }
  } else {
    // CPU path: request step via message-thread timer (RT-safe)
    for (int i = 0; i < numSamples; ++i) {
      if (clock.tick() && isRunning && !isFrozen) {
        cpuStepTimer_.requestStep();
        stepTriggeredThisBlock = true;
      }
    }
  }

  // Auto-reseed logic moved after readLock in stepTriggeredThisBlock check
  if (stepTriggeredThisBlock) {
    // Decrement reseed cooldown
    if (reseedCooldown > 0) {
      --reseedCooldown;
    }
    // GPU needs much higher threshold since readback lags behind simulation
    int stagnationThreshold = isGpu ? 60 : 8;
    if (reseedCooldown == 0 && stagnationCounter >= stagnationThreshold) {
      reseedRng ^= reseedRng << 13;
      reseedRng ^= reseedRng >> 7;
      reseedRng ^= reseedRng << 17;
      currentSeed.store(reseedRng, std::memory_order_relaxed);

      if (isGpu) {
        auto *mgr = &gpuCompute;
        juce::MessageManager::callAsync([mgr, reseedRng = reseedRng]() {
          mgr->seed(reseedRng, 0.3f);
        });
      } else {
        cpuStepTimer_.requestReseed(reseedRng, 0.3f, useSymmetry);
      }
      reseedCooldown = isGpu ? 120 : 0; // GPU needs time for readback
      stagnationCounter = 0;
    }

    // Overpopulation check driven by subsampled grid
    if (reseedCooldown == 0 && overpopCounter >= 3) {
      // Clear and reseed sparsely to break saturation
      reseedRng ^= reseedRng << 13;
      reseedRng ^= reseedRng >> 7;
      reseedRng ^= reseedRng << 17;
      currentSeed.store(reseedRng, std::memory_order_relaxed);
      if (isGpu) {
        auto *mgr = &gpuCompute;
        juce::MessageManager::callAsync([mgr, reseedRng = reseedRng]() {
          mgr->seed(reseedRng, 0.15f);
        });
      } else {
        cpuStepTimer_.requestOverpopReseed(reseedRng, 0.15f, useSymmetry);
      }
      overpopCounter = 0;
      stagnationCounter = 0;
      lastAliveCount = 0;
      reseedCooldown = isGpu ? 120 : 0; // GPU needs time for readback
    }
  }

  // On step: scan grid for active cells, map to notes, trigger voices
  if (stepTriggeredThisBlock) {
    // Read params
    auto waveshapeIdx =
        static_cast<int>(apvts.getRawParameterValue("waveshape")->load());
    auto scaleIdx =
        static_cast<int>(apvts.getRawParameterValue("scale")->load());
    auto keyIdx = static_cast<int>(apvts.getRawParameterValue("key")->load());
    float attack = apvts.getRawParameterValue("attack")->load();
    float hold = apvts.getRawParameterValue("hold")->load();
    float decay = apvts.getRawParameterValue("decay")->load();
    float sustain = apvts.getRawParameterValue("sustain")->load();
    float release = apvts.getRawParameterValue("release")->load();
    float filterCutoff = smoothFilterCutoff.getCurrentValue();
    float filterRes = smoothFilterRes.getCurrentValue();
    int filterModeIdx =
        static_cast<int>(apvts.getRawParameterValue("filterMode")->load());
    float noiseLevel = smoothNoiseLevel.getCurrentValue();
    float subLevel = smoothSubLevel.getCurrentValue();
    int subOctIdx =
        static_cast<int>(apvts.getRawParameterValue("subOctave")->load());
    int maxVoices =
        static_cast<int>(apvts.getRawParameterValue("voiceCount")->load());
    float waveSpread = apvts.getRawParameterValue("waveshapeSpread")->load();
    // Shapes available for cycling (exclude Bell FM = index 7)
    constexpr int kCycleShapeCount = 7;

    quantizer.setScale(static_cast<ScaleQuantizer::Scale>(scaleIdx), keyIdx);

    // --- Fetch locked grid buffers for this step ---
    const float *curData = nullptr;
    const float *prevData = nullptr;
    bool bridgeHasData = bridge.readLock(curData, prevData);
    if (!bridgeHasData)
      bridge.readUnlock();
    
    if (bridgeHasData) {
    int bRows = bridge.getRows();
    int bCols = bridge.getCols();
    uint64_t bGen = bridge.getGeneration();
    
    // Subsample large grids for performance
    // Max cells scanned = 128 * 128 = 16,384. Wait, max grid dimension is rows or cols.
    // If we have 1280x1280, skip = 10, scanning every 10th row/col -> 128x128.
    int skip = std::max(1, std::max(bRows, bCols) / 128);

    // --- Density-driven dynamics ---
    // Recalculate density manually using the locked subsampled grid
    int countAliveBlocked = 0;
    int cellsChecked = 0;
    for (int r = 0; r < bRows; r += skip) {
      for (int c = 0; c < bCols; c += skip) {
        if (GpuGridBridge::isAliveLocked(curData, bRows, bCols, r, c))
          ++countAliveBlocked;
        ++cellsChecked;
      }
    }
    
    // Auto-reseed check logic — only track stagnation when there are alive cells.
    // At GPU warmup or when simulation is truly dead, alive=0 triggers a reseed
    // via the stagnation path below, but we need > 0 before we start counting.
    if (cellsChecked > 0) {
      if (countAliveBlocked == 0) {
        // Simulation is dead — increment stagnation to trigger reseed
        ++stagnationCounter;
        // Don't update lastAliveCount so the first real data will reset
      } else if (countAliveBlocked == lastAliveCount) {
        ++stagnationCounter;
      } else {
        stagnationCounter = 0;
        lastAliveCount = countAliveBlocked;
      }
      // ... overpop cap is based on total cells, we'll scale it.
      int scaledTotal = cellsChecked;
      if (countAliveBlocked > scaledTotal / 2) {
        ++overpopCounter;
      } else {
        overpopCounter = 0;
      }
    }

    float density = (cellsChecked > 0) ? (static_cast<float>(countAliveBlocked) / cellsChecked) : 0.0f;
    
    // Dense grids sound softer; sparse grids are louder
    densityGain = juce::jmap(density, 0.0f, 1.0f, 1.0f, 0.35f);
    smoothDensityGain.setTargetValue(densityGain);
    // Dense grids open the filter; sparse grids keep it tighter
    float densityCutoffMod = juce::jmap(density, 0.0f, 1.0f, 0.5f, 1.0f);
    float modFilterCutoff = filterCutoff * densityCutoffMod;
    
    // Release voices for cells that just died (no longer retrigger everything)
    for (int v = 0; v < kMaxVoices; ++v) {
      if (voices[v].isActive()) {
        int vRow = voices[v].getGridRow();
        int vCol = voices[v].getGridCol();
        if (vRow >= 0 && vCol >= 0) {
          // Cell died or out of grid bounds: release
          if (!GpuGridBridge::isAliveLocked(curData, bRows, bCols, vRow, vCol)) {
            voices[v].noteOff();
          }
        }
      }
    }

    // Trigger new voices only for newly born cells
    int voicesUsed = 0;
    for (int v = 0; v < kMaxVoices; ++v) {
      if (voices[v].isActive())
        ++voicesUsed;
    }

    // Read musicality params
    float noteProb = apvts.getRawParameterValue("noteProbability")->load();
    float velHumanize = apvts.getRawParameterValue("velocityHumanize")->load();
    float melInertia = apvts.getRawParameterValue("melodicInertia")->load();
    float gateTimeFrac = apvts.getRawParameterValue("gateTime")->load();
    float strumSpread = apvts.getRawParameterValue("strumSpread")->load();
    float roundRobin = apvts.getRawParameterValue("roundRobin")->load();

    // Anti-cacophony params
    float consonance = apvts.getRawParameterValue("consonance")->load();
    int maxTrigsParam = static_cast<int>(
        apvts.getRawParameterValue("maxTriggersPerStep")->load());
    int triggerBudgetKnob =
        static_cast<int>(apvts.getRawParameterValue("triggerBudget")->load());
    // Trigger budget: knob override > 0 takes precedence, else engine default
    int maxTrigsPerStep =
        (triggerBudgetKnob > 0)
            ? triggerBudgetKnob
            : std::min(maxTrigsParam, engine->getDefaultTriggerBudget());
    float restProb = apvts.getRawParameterValue("restProbability")->load();
    float engineGainScale = engine->getGainScale();
    float pitchGravity = apvts.getRawParameterValue("pitchGravity")->load();

    // Musicality hard rules params
    int maxLeap = static_cast<int>(
        apvts.getRawParameterValue("maxLeap")->load());
    int baseOctave = static_cast<int>(
        apvts.getRawParameterValue("baseOctave")->load());
    int octaveRange = static_cast<int>(
        apvts.getRawParameterValue("octaveRange")->load());
    bool musicalityBypassed = apvts.getRawParameterValue("musicalityBypass")->load() > 0.5f;

    // Calculate step interval in samples for gate time
    double stepIntervalSec = clock.getStepIntervalSeconds();
    int stepIntervalSamples =
        static_cast<int>(stepIntervalSec * currentSampleRate);

    // --- Rest probability: chance of skipping ALL triggers this step ---
    if (restProb > 0.0f) {
      musicRng ^= musicRng << 13;
      musicRng ^= musicRng >> 7;
      musicRng ^= musicRng << 17;
      float restRoll = static_cast<float>(musicRng & 0xFFFF) / 65535.0f;
      if (restRoll < restProb)
        goto skipTriggers; // Full rest for this step
    }

    // --- Density-adaptive voice count ---
    {
      float gridDensity = bridge.getDensity();
      int effectiveMaxVoices = maxVoices;
      if (gridDensity > 0.3f) {
        // Linearly reduce voices: at 100% density, halve the count
        float reduction = (gridDensity - 0.3f) / 0.7f * 0.5f;
        effectiveMaxVoices =
            std::max(1, maxVoices - static_cast<int>(reduction * maxVoices));
      }

      // Collect currently-active MIDI notes for consonance checking
      int activeNotes[kMaxVoices];
      int activeNoteCount = 0;
      for (int v = 0; v < kMaxVoices; ++v) {
        if (voices[v].isActive() && voices[v].getCurrentNote() > 0) {
          activeNotes[activeNoteCount++] = voices[v].getCurrentNote();
        }
      }

      int triggersThisStep = 0;

      for (int col = 0;
           col < bCols && voicesUsed < effectiveMaxVoices &&
           triggersThisStep < maxTrigsPerStep;
           col += skip) {
        for (int row = 0; row < bRows; row += skip) {
          if (GpuGridBridge::wasBornLocked(curData, prevData, bRows, bCols, row, col, bGen)) {
            // --- Note probability: skip trigger randomly ---
            musicRng ^= musicRng << 13;
            musicRng ^= musicRng >> 7;
            musicRng ^= musicRng << 17;
            float roll = static_cast<float>(musicRng & 0xFFFF) / 65535.0f;
            if (roll > noteProb)
              break; // Skip this column entirely

            // --- Pitch generation (scale quantization always active) ---
            int midiNote;

            if (!musicalityBypassed) {
              // --- Melodic inertia: repeat or stepwise motion ---
              musicRng ^= musicRng << 13;
              musicRng ^= musicRng >> 7;
              musicRng ^= musicRng << 17;
              float inertiaRoll =
                  static_cast<float>(musicRng & 0xFFFF) / 65535.0f;
              if (inertiaRoll < melInertia && lastTriggeredMidiNote > 0) {
                // 50% exact repeat, 50% stepwise motion
                musicRng ^= musicRng << 13;
                musicRng ^= musicRng >> 7;
                musicRng ^= musicRng << 17;
                float stepRoll =
                    static_cast<float>(musicRng & 0xFFFF) / 65535.0f;
                if (stepRoll < 0.5f) {
                  midiNote = lastTriggeredMidiNote; // Exact repeat
                } else {
                  // Stepwise: walk +-1 or +-2 scale degrees
                  musicRng ^= musicRng << 13;
                  musicRng ^= musicRng >> 7;
                  musicRng ^= musicRng << 17;
                  float stepsRoll =
                      static_cast<float>(musicRng & 0xFFFF) / 65535.0f;
                  int steps = (stepsRoll < 0.7f) ? 1 : 2;
                  int stepped = lastTriggeredMidiNote;
                  for (int s = 0; s < steps; ++s) {
                    int next = quantizer.quantizeToNearest(
                        stepped + lastMelodicDirection_);
                    if (next == stepped)
                      next = quantizer.quantizeToNearest(
                          stepped + lastMelodicDirection_ * 2);
                    stepped = next;
                  }
                  midiNote = stepped;
                  // 15% chance of direction flip
                  musicRng ^= musicRng << 13;
                  musicRng ^= musicRng >> 7;
                  musicRng ^= musicRng << 17;
                  float flipRoll =
                      static_cast<float>(musicRng & 0xFFFF) / 65535.0f;
                  if (flipRoll < 0.15f)
                    lastMelodicDirection_ *= -1;
                }
              } else if (pitchGravity > 0.0f) {
                // --- Pitch gravity: bias toward chord tones ---
                midiNote = quantizer.quantizeWeighted(
                    row, col, baseOctave, octaveRange, bCols, pitchGravity, musicRng);
              } else {
                midiNote = quantizer.quantize(row, col, baseOctave, octaveRange, bCols);
              }

              // --- Consonance filter: weighted dissonance scoring ---
              if (consonance > 0.0f && activeNoteCount > 0) {
                int dissThreshold = static_cast<int>((1.0f - consonance) * 3.0f) + 1;
                int dissScore = ScaleQuantizer::scoreDissAgainstAll(
                    midiNote, activeNotes, activeNoteCount);
                if (dissScore >= dissThreshold) {
                  int above = quantizer.quantizeToNearest(midiNote + 1);
                  int below = quantizer.quantizeToNearest(midiNote - 1);
                  int scoreAbove = ScaleQuantizer::scoreDissAgainstAll(
                      above, activeNotes, activeNoteCount);
                  int scoreBelow = ScaleQuantizer::scoreDissAgainstAll(
                      below, activeNotes, activeNoteCount);
                  if (scoreAbove < dissScore && scoreAbove <= scoreBelow)
                    midiNote = above;
                  else if (scoreBelow < dissScore)
                    midiNote = below;
                }
              }

              // --- Max leap clamping ---
              if (maxLeap > 0) {
                midiNote = quantizer.clampLeap(midiNote, lastTriggeredMidiNote, maxLeap);
              }
            } else {
              // Bypassed: raw scale-quantized output only
              midiNote = quantizer.quantize(row, col, baseOctave, octaveRange, bCols);
            }

            // --- Musical range clamp: C2(36) to C7(96) safety ---
            if (midiNote < 36) midiNote = 36;
            if (midiNote > 96) midiNote = 96;

            lastTriggeredMidiNote = midiNote;

            // --- Pitch histogram: decaying key detection (RT-safe) ---
            for (int i = 0; i < 12; ++i) noteHistogram_[i] *= 0.995f;
            noteHistogram_[midiNote % 12] += 1.0f;
            float maxW = 0.0f;
            for (int i = 0; i < 12; ++i) {
              if (noteHistogram_[i] > maxW) {
                maxW = noteHistogram_[i];
                detectedKey_ = i;
              }
            }

            float frequency = tuning.getFrequency(midiNote);

            // --- Velocity humanization + engine intensity ---
            float vel = lastMidiVelocity;

            // Engine-specific intensity modulates velocity
              float cellIntensity = GpuGridBridge::getCellIntensityLocked(curData, bRows, bCols, row, col);
            vel *= cellIntensity;

            if (velHumanize > 0.0f) {
              musicRng ^= musicRng << 13;
              musicRng ^= musicRng >> 7;
              musicRng ^= musicRng << 17;
              float velOffset =
                  (static_cast<float>(musicRng & 0xFFFF) / 65535.0f - 0.5f) *
                  2.0f * velHumanize;
              vel = std::clamp(vel + velOffset, 0.1f, 1.0f);
            }

            // Apply per-engine gain scale to prevent energy buildup
            vel *= engineGainScale;

            // Find a free voice (round-robin: rotate start index)
            int voiceIdx = -1;
            int searchStart = 0;
            if (roundRobin > 0.0f) {
              musicRng ^= musicRng << 13;
              musicRng ^= musicRng >> 7;
              musicRng ^= musicRng << 17;
              float rrRoll = static_cast<float>(musicRng & 0xFFFF) / 65535.0f;
              if (rrRoll < roundRobin)
                searchStart = roundRobinIndex;
            }
            for (int i = 0; i < kMaxVoices; ++i) {
              int v = (searchStart + i) % kMaxVoices;
              if (!voices[v].isActive()) {
                voiceIdx = v;
                break;
              }
            }
            roundRobinIndex = (roundRobinIndex + 1) % kMaxVoices;
            // Steal quietest if no free voice
            if (voiceIdx < 0) {
              double quietest = 999.0;
              for (int v = 0; v < kMaxVoices; ++v) {
                if (voices[v].getEnvelopeLevel() < quietest) {
                  quietest = voices[v].getEnvelopeLevel();
                  voiceIdx = v;
                }
              }
            }

            if (voiceIdx >= 0) {
              // Waveshape spread: 0 = all voices use selected shape,
              // 1 = cycle through shapes (Bell FM excluded from cycling)
              int shapeIdx = waveshapeIdx;
              if (waveSpread > 0.0f) {
                musicRng ^= musicRng << 13;
                musicRng ^= musicRng >> 7;
                musicRng ^= musicRng << 17;
                float spreadRoll =
                    static_cast<float>(musicRng & 0xFFFF) / 65535.0f;
                if (spreadRoll < waveSpread) {
                  shapeIdx = (waveshapeIdx + col) % kCycleShapeCount;
                }
              }
              auto shape = static_cast<PolyBLEPOscillator::Shape>(shapeIdx);
              voices[voiceIdx].setWaveshape(shape);
              voices[voiceIdx].setEnvelopeParams(attack, hold, decay, sustain,
                                                 release, currentSampleRate);
              voices[voiceIdx].setFilterCutoff(modFilterCutoff);
              voices[voiceIdx].setFilterResonance(filterRes);
              voices[voiceIdx].setFilterMode(
                  static_cast<SVFilter::Mode>(filterModeIdx));
              voices[voiceIdx].setNoiseLevel(noiseLevel);
              voices[voiceIdx].setSubLevel(subLevel);
              voices[voiceIdx].setSubOctave(
                  static_cast<SubOscillator::OctaveMode>(subOctIdx));

              double pan = (bCols > 1)
                               ? (2.0 * col / (bCols - 1) - 1.0)
                               : 0.0;
              pan *= static_cast<double>(
                  apvts.getRawParameterValue("stereoWidth")->load());
              voices[voiceIdx].setPan(pan);

              voices[voiceIdx].setGridPosition(row, col);

              // --- Gate time: set per-voice auto-release countdown ---
              if (gateTimeFrac < 1.0f && stepIntervalSamples > 0) {
                int gateSamples =
                    static_cast<int>(gateTimeFrac * stepIntervalSamples);
                if (gateSamples < 1)
                  gateSamples = 1;
                voices[voiceIdx].setGateTime(gateSamples);
              }

              // --- Strum spread: onset delay per column position ---
              if (strumSpread > 0.0f) {
                float colFrac = (bCols > 1) ? static_cast<float>(col) /
                                                           (bCols - 1)
                                                     : 0.0f;
                int delaySamples = static_cast<int>(colFrac * strumSpread *
                                                    0.001f * currentSampleRate);
                voices[voiceIdx].setOnsetDelay(delaySamples);
              }

              voices[voiceIdx].noteOn(midiNote, vel, frequency,
                                      currentSampleRate);
              ++voicesUsed;
              ++triggersThisStep;

              // Add to active notes for consonance checking of subsequent
              // triggers
              if (activeNoteCount < kMaxVoices)
                activeNotes[activeNoteCount++] = midiNote;
            }
            break; // One note per column
          }
        }
      }
    }
    bridge.readUnlock();
    } // end if (bridgeHasData)
  skipTriggers:;
  }

  // Render all active voices per-sample
  for (int sample = 0; sample < numSamples; ++sample) {
    double mixL = 0.0;
    double mixR = 0.0;
    for (int v = 0; v < kMaxVoices; ++v) {
      if (voices[v].isActive()) {
        auto stereo = voices[v].renderNextSample();
        mixL += stereo.left;
        mixR += stereo.right;
      }
    }

    // Adaptive gain staging: normalize by active voice count
    int activeVoiceCount = 0;
    for (int v = 0; v < kMaxVoices; ++v) {
      if (voices[v].isActive())
        ++activeVoiceCount;
    }
    const double normFactor =
        (activeVoiceCount > 1)
            ? 1.0 / std::sqrt(static_cast<double>(activeVoiceCount))
            : 1.0;
    const double dGain =
        static_cast<double>(smoothDensityGain.getNextValue()) * normFactor;
    if (buffer.getNumChannels() >= 2) {
      buffer.setSample(0, sample, static_cast<float>(mixL * dGain));
      buffer.setSample(1, sample, static_cast<float>(mixR * dGain));
    } else if (buffer.getNumChannels() >= 1) {
      buffer.setSample(0, sample,
                       static_cast<float>((mixL + mixR) * 0.5 * dGain));
    }
  }

  // --- Effect on/off toggles ---
  chorus.setBypass(!apvts.getRawParameterValue("chorusOn")->load());
  delay.setBypass(!apvts.getRawParameterValue("delayOn")->load());
  reverb.setBypass(!apvts.getRawParameterValue("reverbOn")->load());
  phaser.setBypass(!apvts.getRawParameterValue("phaserOn")->load());
  flanger.setBypass(!apvts.getRawParameterValue("flangerOn")->load());
  bitcrush.setBypass(!apvts.getRawParameterValue("bitcrushOn")->load());
  tapeSat.setBypass(!apvts.getRawParameterValue("tapeOn")->load());
  shimmer.setBypass(!apvts.getRawParameterValue("shimmerOn")->load());
  pingPong.setBypass(!apvts.getRawParameterValue("pingPongOn")->load());

  // --- Modulation LFOs ---
  modLfo1.setShape(
      static_cast<int>(apvts.getRawParameterValue("lfo1Shape")->load()));
  modLfo1.setRate(apvts.getRawParameterValue("lfo1Rate")->load());
  float lfo1Amount = apvts.getRawParameterValue("lfo1Amount")->load();
  int lfo1Dest =
      static_cast<int>(apvts.getRawParameterValue("lfo1Dest")->load());
  modLfo2.setShape(
      static_cast<int>(apvts.getRawParameterValue("lfo2Shape")->load()));
  modLfo2.setRate(apvts.getRawParameterValue("lfo2Rate")->load());
  float lfo2Amount = apvts.getRawParameterValue("lfo2Amount")->load();
  int lfo2Dest =
      static_cast<int>(apvts.getRawParameterValue("lfo2Dest")->load());

  // Tick LFOs once per block and compute modulation offsets
  float lfo1Val = modLfo1.tickBlock(numSamples) * lfo1Amount;
  float lfo2Val = modLfo2.tickBlock(numSamples) * lfo2Amount;

  // Modulation destination map:
  // 0=None, 1=chorusMix, 2=chorusRate, 3=delayMix, 4=delayTime,
  // 5=reverbMix, 6=phaserRate, 7=flangerRate, 8=bitcrushBits,
  // 9=tapeDrive, 10=shimmerMix, 11=pingPongTime, 12=filterCutoff,
  // 13=filterRes, 14=stereoWidth, 15=noiseLevel, 16=attack, 17=decay
  float modOffsets[18] = {};
  if (lfo1Dest > 0 && lfo1Dest < 18)
    modOffsets[lfo1Dest] += lfo1Val;
  if (lfo2Dest > 0 && lfo2Dest < 18)
    modOffsets[lfo2Dest] += lfo2Val;

  // --- Read effect parameters (with modulation offsets) ---
  auto clamp01 = [](float v) { return std::max(0.0f, std::min(1.0f, v)); };
  const float chorusMixP =
      clamp01(apvts.getRawParameterValue("chorusMix")->load() + modOffsets[1]);
  const float delayMixP =
      clamp01(apvts.getRawParameterValue("delayMix")->load() + modOffsets[3]);
  const float reverbMixP =
      clamp01(apvts.getRawParameterValue("reverbMix")->load() + modOffsets[5]);
  const float phaserMixP =
      clamp01(apvts.getRawParameterValue("phaserMix")->load());
  const float flangerMixP =
      clamp01(apvts.getRawParameterValue("flangerMix")->load());
  const float bitcrushMixP =
      clamp01(apvts.getRawParameterValue("bitcrushMix")->load());
  const float tapeMixP = clamp01(apvts.getRawParameterValue("tapeMix")->load());
  const float shimmerMixP = clamp01(
      apvts.getRawParameterValue("shimmerMix")->load() + modOffsets[10]);
  const float pingPongMixP =
      clamp01(apvts.getRawParameterValue("pingPongMix")->load());

  // Configure effects (parameter reads with modulation, not per-sample)
  chorus.setRate(
      std::max(0.1f, apvts.getRawParameterValue("chorusRate")->load() +
                         modOffsets[2] * 5.0f));
  chorus.setDepth(apvts.getRawParameterValue("chorusDepth")->load());
  chorus.setMix(chorusMixP);

  delay.setTime(
      std::max(0.01f, apvts.getRawParameterValue("delayTime")->load() +
                          modOffsets[4] * 2.0f));
  delay.setFeedback(apvts.getRawParameterValue("delayFeedback")->load());
  delay.setMix(delayMixP);

  reverb.setDecay(apvts.getRawParameterValue("reverbDecay")->load());
  reverb.setDamping(apvts.getRawParameterValue("reverbDamping")->load());
  reverb.setMix(reverbMixP);

  phaser.setRate(apvts.getRawParameterValue("phaserRate")->load());
  phaser.setDepth(apvts.getRawParameterValue("phaserDepth")->load());
  phaser.setMix(phaserMixP);

  flanger.setRate(apvts.getRawParameterValue("flangerRate")->load());
  flanger.setDepth(apvts.getRawParameterValue("flangerDepth")->load());
  flanger.setMix(flangerMixP);

  bitcrush.setBitDepth(apvts.getRawParameterValue("bitcrushBits")->load());
  bitcrush.setDownsample(apvts.getRawParameterValue("bitcrushRate")->load());
  bitcrush.setMix(bitcrushMixP);

  tapeSat.setDrive(apvts.getRawParameterValue("tapeDrive")->load());
  tapeSat.setTone(apvts.getRawParameterValue("tapeTone")->load());
  tapeSat.setMix(tapeMixP);

  shimmer.setDecay(apvts.getRawParameterValue("shimmerDecay")->load());
  shimmer.setShimmer(apvts.getRawParameterValue("shimmerAmount")->load());
  shimmer.setMix(shimmerMixP);

  pingPong.setTime(apvts.getRawParameterValue("pingPongTime")->load());
  pingPong.setFeedback(apvts.getRawParameterValue("pingPongFeedback")->load());
  pingPong.setMix(pingPongMixP);

  // --- Apply effects chain (parallel send/return architecture) ---
  bool anyEffectActive =
      chorusMixP > 0.0f || delayMixP > 0.0f || reverbMixP > 0.0f ||
      phaserMixP > 0.0f || flangerMixP > 0.0f || bitcrushMixP > 0.0f ||
      tapeMixP > 0.0f || shimmerMixP > 0.0f || pingPongMixP > 0.0f;

  if (buffer.getNumChannels() >= 2 && anyEffectActive) {
    // Soft-clip function to tame hot signals before they enter effects
    auto softClip = [](float x) -> float {
      if (x > 1.5f)
        return 1.0f;
      if (x < -1.5f)
        return -1.0f;
      return x - (x * x * x) / 6.75f;
    };

    for (int sample = 0; sample < numSamples; ++sample) {
      float dryL = buffer.getSample(0, sample);
      float dryR = buffer.getSample(1, sample);

      // Soft-clip before feeding effects
      float clippedL = softClip(dryL);
      float clippedR = softClip(dryR);

      float outL, outR;
      effectChain.processParallel(clippedL, clippedR, outL, outR);

      buffer.setSample(0, sample, outL);
      buffer.setSample(1, sample, outR);
    }
  }

  // --- Safety limiter (brick-wall, always active) ---
  {
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    safetyLimiter.process(ctx);
  }

  // --- SafetyProcessor (DC filter + ultrasonic LP + brickwall, always active)
  // ---
  if (buffer.getNumChannels() >= 2) {
    for (int sample = 0; sample < numSamples; ++sample) {
      float L = buffer.getSample(0, sample);
      float R = buffer.getSample(1, sample);
      safetyProc.process(L, R);
      buffer.setSample(0, sample, L);
      buffer.setSample(1, sample, R);
    }
  }

  // Update grid snapshot for UI thread (only when bridge has new data)
  // Throttle conversion for large grids to prevent audio thread overload:
  //   >500K cells: convert every 4th generation
  //   >100K cells: convert every 2nd generation
  uint64_t bridgeGen = bridge.getGeneration();
  if (bridgeGen != lastSnapshotGeneration_) {
    int totalCells = bridge.getRows() * bridge.getCols();
    int convertSkip = (totalCells > 500000) ? 4 : (totalCells > 100000) ? 2 : 1;
    bool shouldConvert = (bridgeGen % convertSkip == 0);
    lastSnapshotGeneration_ = bridgeGen;
    if (shouldConvert) {
      int backIdx = 1 - gridReadIdx_.load(std::memory_order_relaxed);
      bridge.convertToGrid(gridSnapshots_[backIdx]);
      gridReadIdx_.store(backIdx, std::memory_order_release);
    }
  }
  engineGeneration.store(engine->getGeneration(), std::memory_order_relaxed);

  // Apply master volume with smoothing (per-sample)
  for (int sample = 0; sample < numSamples; ++sample) {
    const float vol = masterVolume.getNextValue();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
      buffer.setSample(ch, sample, buffer.getSample(ch, sample) * vol);
  }

  // CPU load measurement
  auto endTime = juce::Time::getHighResolutionTicks();
  double elapsedSeconds =
      juce::Time::highResolutionTicksToSeconds(endTime - startTime);
  double budgetSeconds = static_cast<double>(numSamples) / currentSampleRate;
  cpuLoadPercent.store(
      static_cast<float>(elapsedSeconds / budgetSeconds * 100.0),
      std::memory_order_relaxed);
}

//==============================================================================
juce::AudioProcessorEditor *AlgoNebulaProcessor::createEditor() {
  return new AlgoNebulaEditor(*this);
}

//==============================================================================
void AlgoNebulaProcessor::getStateInformation(juce::MemoryBlock &destData) {
  auto state = apvts.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());

  // Append grid state as child element
  auto *gridXml = xml->createNewChildElement("GridState");
  gridXml->setAttribute("version", 1);
  gridXml->setAttribute("algorithm", lastAlgorithmIdx);
  gridXml->setAttribute("gridSize", lastGridSizeIdx);
  gridXml->setAttribute("seed",
                        juce::String(static_cast<juce::int64>(
                            currentSeed.load(std::memory_order_relaxed))));

  // Encode grid cells and ages as base64
  const auto &grid = engine->getGrid();
  int rows = grid.getRows();
  int cols = grid.getCols();
  gridXml->setAttribute("rows", rows);
  gridXml->setAttribute("cols", cols);

  // Cell data: pack only the active region (rows * cols bytes)
  juce::MemoryBlock cellBlock(static_cast<size_t>(rows * cols));
  for (int r = 0; r < rows; ++r)
    for (int c = 0; c < cols; ++c)
      static_cast<uint8_t *>(cellBlock.getData())[r * cols + c] =
          grid.getCell(r, c);
  gridXml->setAttribute("cells", cellBlock.toBase64Encoding());

  // Age data: pack as little-endian uint16 (rows * cols * 2 bytes)
  juce::MemoryBlock ageBlock(static_cast<size_t>(rows * cols * 2));
  auto *agePtr = static_cast<uint8_t *>(ageBlock.getData());
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      uint16_t age = grid.getAge(r, c);
      int idx = (r * cols + c) * 2;
      agePtr[idx] = static_cast<uint8_t>(age & 0xFF);
      agePtr[idx + 1] = static_cast<uint8_t>((age >> 8) & 0xFF);
    }
  }
  gridXml->setAttribute("ages", ageBlock.toBase64Encoding());

  copyXmlToBinary(*xml, destData);
}

void AlgoNebulaProcessor::setStateInformation(const void *data,
                                              int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
  if (xml == nullptr || !xml->hasTagName(apvts.state.getType()))
    return;

  // Restore APVTS parameters
  apvts.replaceState(juce::ValueTree::fromXml(*xml));

  // Restore grid state if present
  auto *gridXml = xml->getChildByName("GridState");
  if (gridXml == nullptr)
    return; // Legacy state without grid ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â just use APVTS defaults

  int version = gridXml->getIntAttribute("version", 0);
  if (version < 1)
    return; // Unknown version ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â skip grid restore

  // Restore engine type and seed
  int algoIdx = gridXml->getIntAttribute("algorithm", 0);
  int gridSizeIdx = gridXml->getIntAttribute("gridSize", 1);
  auto seed = static_cast<uint64_t>(
      gridXml->getStringAttribute("seed", "12345").getLargeIntValue());

  int rows = gridXml->getIntAttribute("rows", 12);
  int cols = gridXml->getIntAttribute("cols", 16);

  // Validate dimensions
  if (rows < 1 || rows > Grid::kMaxRows || cols < 1 || cols > Grid::kMaxCols)
    return;

  // Recreate engine
  lastAlgorithmIdx = algoIdx;
  lastGridSizeIdx = gridSizeIdx;
  engine = createEngine(algoIdx, rows, cols);
  currentSeed.store(seed, std::memory_order_relaxed);

  // Decode cell data
  juce::String cellB64 = gridXml->getStringAttribute("cells", "");
  if (cellB64.isNotEmpty()) {
    juce::MemoryBlock cellBlock;
    if (cellBlock.fromBase64Encoding(cellB64) &&
        cellBlock.getSize() >= static_cast<size_t>(rows * cols)) {
      auto &grid = engine->getGridMutable();
      const auto *cellData = static_cast<const uint8_t *>(cellBlock.getData());
      for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
          grid.setCell(r, c, cellData[r * cols + c]);
    }
  }

  // Decode age data
  juce::String ageB64 = gridXml->getStringAttribute("ages", "");
  if (ageB64.isNotEmpty()) {
    juce::MemoryBlock ageBlock;
    if (ageBlock.fromBase64Encoding(ageB64) &&
        ageBlock.getSize() >= static_cast<size_t>(rows * cols * 2)) {
      auto &grid = engine->getGridMutable();
      const auto *ageData = static_cast<const uint8_t *>(ageBlock.getData());
      for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int idx = (r * cols + c) * 2;
            uint16_t age = static_cast<uint16_t>(ageData[idx]) |
                           (static_cast<uint16_t>(ageData[idx + 1]) << 8);
            grid.setAge(r, c, age);
          }
        }
      }
    }

  // Update bridge and snapshot from restored engine state
  gpuCompute.getBridge().updateFromCpu(engine->getGrid());
  {
    int backIdx = 1 - gridReadIdx_.load(std::memory_order_relaxed);
    gpuCompute.getBridge().convertToGrid(gridSnapshots_[backIdx]);
    gridReadIdx_.store(backIdx, std::memory_order_release);
  }
  stagnationCounter = 0;
  lastAliveCount = 0;
}

//==============================================================================
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new AlgoNebulaProcessor();
}