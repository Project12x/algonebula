#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AlgoNebulaProcessor::AlgoNebulaProcessor()
    : AudioProcessor(BusesProperties().withOutput(
          "Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "AlgoNebulaState", createParameterLayout()) {}

AlgoNebulaProcessor::~AlgoNebulaProcessor() = default;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
AlgoNebulaProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  // --- Master ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("masterVolume", 1), "Master Volume",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.7f));

  // --- Algorithm ---
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("algorithm", 1), "Algorithm",
      juce::StringArray{"Game of Life", "Wolfram 1D", "Brian's Brain",
                        "Cyclic CA", "Reaction-Diffusion", "Particle Swarm",
                        "Lenia", "Brownian Field"},
      0));

  // --- Clock ---
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
      juce::ParameterID("voiceCount", 1), "Voice Count", 1, 8, 4));

  // --- Waveshape ---
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("waveshape", 1), "Waveshape",
      juce::StringArray{"Sine", "Triangle", "Saw", "Pulse", "Sine+Oct",
                        "Fifth Stack", "Pad", "Bell"},
      0));

  // --- Ambient ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("droneSustain", 1), "Drone Sustain",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("noteProbability", 1), "Note Probability",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

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
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("roundRobin", 1), "Round Robin",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.2f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("velocityHumanize", 1), "Velocity Humanize",
      juce::NormalisableRange<float>(0.0f, 0.3f, 0.01f), 0.05f));

  // --- Envelope ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("attack", 1), "Attack",
      juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.5f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("release", 1), "Release",
      juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 3.0f));

  // --- Tuning ---
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("tuning", 1), "Tuning",
      juce::StringArray{"12-TET", "Just Intonation", "Pythagorean"}, 0));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("refPitch", 1), "Reference Pitch",
      juce::NormalisableRange<float>(420.0f, 460.0f, 0.1f), 440.0f));

  return layout;
}

//==============================================================================
void AlgoNebulaProcessor::prepareToPlay(double sampleRate,
                                        int samplesPerBlock) {
  currentSampleRate = sampleRate;
  currentBlockSize = samplesPerBlock;

  // Pre-allocate stereo mix buffer (no allocations on audio thread)
  stereoMixBuffer.setSize(2, samplesPerBlock, false, true, false);

  // Initialize smoothed parameters
  masterVolume.reset(sampleRate, 0.02); // 20ms smoothing
  masterVolume.setCurrentAndTargetValue(
      apvts.getRawParameterValue("masterVolume")->load());
}

void AlgoNebulaProcessor::releaseResources() {
  // Nothing to release in Phase 1
}

void AlgoNebulaProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                       juce::MidiBuffer &midiMessages) {
  auto startTime = juce::Time::getHighResolutionTicks();

  juce::ScopedNoDenormals noDenormals;

  // Read smoothed parameters
  masterVolume.setTargetValue(
      apvts.getRawParameterValue("masterVolume")->load());

  const int numSamples = buffer.getNumSamples();

  // Clear output buffer
  for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    buffer.clear(ch, 0, numSamples);

  // TODO Phase 2+: Engine -> Quantizer -> Voices -> FX -> Safety -> Output

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
  copyXmlToBinary(*xml, destData);
}

void AlgoNebulaProcessor::setStateInformation(const void *data,
                                              int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
  if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

//==============================================================================
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new AlgoNebulaProcessor();
}
