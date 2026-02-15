#pragma once

#include "PluginProcessor.h"
#include "ui/NebulaLookAndFeel.h"
#include <juce_audio_utils/juce_audio_utils.h>

/// AlgoNebula editor — Phase 1 skeleton with resizable dark panel,
/// Nebula LookAndFeel, CPU meter, and algorithm/scale selectors.
class AlgoNebulaEditor : public juce::AudioProcessorEditor,
                         private juce::Timer {
public:
  explicit AlgoNebulaEditor(AlgoNebulaProcessor &);
  ~AlgoNebulaEditor() override;

  void paint(juce::Graphics &) override;
  void resized() override;

private:
  void timerCallback() override;

  AlgoNebulaProcessor &processor;

  NebulaLookAndFeel nebulaLnF;

  // --- Controls ---
  juce::Slider masterVolumeSlider;
  juce::Label masterVolumeLabel;

  juce::ComboBox algorithmSelector;
  juce::Label algorithmLabel;

  juce::ComboBox scaleSelector;
  juce::Label scaleLabel;

  juce::ComboBox keySelector;
  juce::Label keyLabel;

  juce::ComboBox waveshapeSelector;
  juce::Label waveshapeLabel;

  juce::Label cpuMeterLabel;

  // --- APVTS Attachments ---
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      masterVolumeAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      algorithmAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      scaleAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      keyAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      waveshapeAttach;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlgoNebulaEditor)
};
