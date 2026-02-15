#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>


#include "engine/CellEditQueue.h"
#include "engine/CellularEngine.h"
#include "engine/GameOfLife.h"
#include "engine/Grid.h"


// Forward declarations
class NebulaLookAndFeel;

//==============================================================================
/// AlgoNebula processor — generative ambient synthesizer.
/// Phase 2: CellularEngine + Game of Life with double-buffered grid.
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
  GameOfLife engine{12, 16, GameOfLife::RulePreset::Classic};
  Grid gridSnapshot; // Double-buffer: audio writes, GL/UI reads
  CellEditQueue cellEditQueue;
  std::atomic<uint64_t> engineGeneration{0};

  // --- Performance monitoring ---
  std::atomic<float> cpuLoadPercent{0.0f};
  double currentSampleRate = 44100.0;
  int currentBlockSize = 512;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlgoNebulaProcessor)
};
