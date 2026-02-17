#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>

#include "engine/BriansBrain.h"
#include "engine/BrownianField.h"
#include "engine/CellEditQueue.h"
#include "engine/CellularEngine.h"
#include "engine/ClockDivider.h"
#include "engine/CyclicCA.h"
#include "engine/GameOfLife.h"
#include "engine/Grid.h"
#include "engine/LeniaEngine.h"
#include "engine/Microtuning.h"
#include "engine/ParticleSwarm.h"
#include "engine/ReactionDiffusion.h"
#include "engine/ScaleQuantizer.h"
#include "engine/SynthVoice.h"

// Forward declarations
class NebulaLookAndFeel;

//==============================================================================
/// AlgoNebula processor — generative ambient synthesizer.
/// Phase 4: PolyBLEP synth voices + AHDSR + SVF filter.
class AlgoNebulaProcessor : public juce::AudioProcessor {
public:
  AlgoNebulaProcessor();
  ~AlgoNebulaProcessor() override;

  //--- AudioProcessor overrides ---
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override { return JucePlugin_Name; }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return true; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String &) override {}

  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  //--- Parameter tree ---
  juce::AudioProcessorValueTreeState &getAPVTS() { return apvts; }

  // --- Thread-safe metrics ---
  float getCpuLoadPercent() const {
    return cpuLoadPercent.load(std::memory_order_relaxed);
  }

  // --- Engine access (UI thread reads grid snapshot, pushes edits) ---
  const Grid &getGridSnapshot() const { return gridSnapshot; }
  CellEditQueue &getCellEditQueue() { return cellEditQueue; }
  uint64_t getGeneration() const {
    return engineGeneration.load(std::memory_order_relaxed);
  }

  // --- Transport controls (atomic, UI thread writes, audio thread reads) ---
  std::atomic<bool> engineRunning{true};
  std::atomic<bool> clearRequested{false};
  std::atomic<bool> reseedSymmetricRequested{false};

  // --- Seed access ---
  uint64_t getSeed() const {
    return currentSeed.load(std::memory_order_relaxed);
  }
  void setSeed(uint64_t seed) {
    currentSeed.store(seed, std::memory_order_relaxed);
    seedChanged.store(true, std::memory_order_relaxed);
  }

  // --- MIDI keyboard (for virtual keyboard in editor) ---
  juce::MidiKeyboardState &getKeyboardState() { return keyboardState; }

private:
  //--- Parameter layout ---
  static juce::AudioProcessorValueTreeState::ParameterLayout
  createParameterLayout();

  juce::AudioProcessorValueTreeState apvts;

  //--- Smoothed parameters (read by audio thread) ---
  juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> masterVolume;

  // --- Pre-allocated audio buffers ---
  juce::AudioBuffer<float> stereoMixBuffer;

  // --- Cellular Engine ---
  std::unique_ptr<CellularEngine> engine;
  static std::unique_ptr<CellularEngine> createEngine(int algoIdx);
  Grid gridSnapshot; // Double-buffer: audio writes, GL/UI reads
  CellEditQueue cellEditQueue;
  std::atomic<uint64_t> engineGeneration{0};

  // --- Clock + Music Theory ---
  ClockDivider clock;
  ScaleQuantizer quantizer;
  Microtuning tuning;

  // --- Synth Voices ---
  static constexpr int kMaxVoices = 8;
  SynthVoice voices[kMaxVoices];
  bool stepTriggeredThisBlock = false;

  // --- MIDI keyboard state ---
  juce::MidiKeyboardState keyboardState;
  float lastMidiVelocity = 0.8f;

  // --- Auto-reseed stagnation tracking ---
  int lastAliveCount = 0;
  int stagnationCounter = 0;
  int overpopCounter = 0;
  uint64_t reseedRng = 12345;
  std::atomic<uint64_t> currentSeed{12345};
  std::atomic<bool> seedChanged{false};

  // --- Algorithm tracking ---
  int lastAlgorithmIdx = 0;
  float densityGain = 1.0f; // Updated each step from grid density

public:
  // UI accessors (call from message thread only)
  const CellularEngine &getEngine() const { return *engine; }
  float getDensityGain() const { return densityGain; }

private:
  // --- Performance monitoring ---
  std::atomic<float> cpuLoadPercent{0.0f};
  double currentSampleRate = 44100.0;
  int currentBlockSize = 512;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlgoNebulaProcessor)
};
