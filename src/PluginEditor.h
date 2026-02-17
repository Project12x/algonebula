#pragma once

#include "PluginProcessor.h"
#include "ui/GridComponent.h"
#include "ui/NebulaLookAndFeel.h"
#include <juce_audio_utils/juce_audio_utils.h>

/// AlgoNebula editor — Phase 4.5 with grid visualization, full synth controls.
class AlgoNebulaEditor : public juce::AudioProcessorEditor,
                         private juce::Timer {
public:
  explicit AlgoNebulaEditor(AlgoNebulaProcessor &);
  ~AlgoNebulaEditor() override;

  void paint(juce::Graphics &) override;
  void resized() override;

private:
  void timerCallback() override;

  // Helper to create a labeled rotary knob
  struct LabeledKnob {
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        attach;
  };

  // Helper to create a labeled combo box
  struct LabeledCombo {
    juce::ComboBox combo;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        attach;
  };

  void setupKnob(LabeledKnob &knob, const juce::String &labelText,
                 const juce::String &paramID);
  void setupCombo(LabeledCombo &combo, const juce::String &labelText,
                  const juce::String &paramID);
  void layoutKnobRow(juce::Rectangle<int> area,
                     std::initializer_list<LabeledKnob *> knobs);
  void layoutComboRow(juce::Rectangle<int> area,
                      std::initializer_list<LabeledCombo *> combos);

  AlgoNebulaProcessor &processor;
  NebulaLookAndFeel nebulaLnF;

  // --- Grid ---
  GridComponent gridComponent;

  // --- Top selectors ---
  LabeledCombo algorithmCombo;
  LabeledCombo scaleCombo;
  LabeledCombo keyCombo;
  LabeledCombo waveshapeCombo;

  // --- Clock ---
  LabeledKnob bpmKnob;
  LabeledCombo clockDivCombo;
  LabeledKnob swingKnob;

  // --- Envelope ---
  LabeledKnob attackKnob;
  LabeledKnob holdKnob;
  LabeledKnob decayKnob;
  LabeledKnob sustainKnob;
  LabeledKnob releaseKnob;

  // --- Filter ---
  LabeledKnob filterCutoffKnob;
  LabeledKnob filterResKnob;
  LabeledCombo filterModeCombo;

  // --- Mix ---
  LabeledKnob noiseLevelKnob;
  LabeledKnob subLevelKnob;
  LabeledCombo subOctaveCombo;

  // --- Tuning ---
  LabeledCombo tuningCombo;
  LabeledKnob refPitchKnob;

  // --- Ambient ---
  LabeledKnob droneSustainKnob;
  LabeledKnob noteProbKnob;
  LabeledKnob gateTimeKnob;

  // --- Humanize ---
  LabeledKnob strumSpreadKnob;
  LabeledKnob melodicInertiaKnob;
  LabeledKnob roundRobinKnob;
  LabeledKnob velHumanizeKnob;

  // --- Global ---
  LabeledKnob masterVolumeKnob;
  LabeledKnob voiceCountKnob;

  // --- Status ---
  juce::Label cpuMeterLabel;

  // --- MIDI Keyboard ---
  juce::MidiKeyboardComponent midiKeyboard;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlgoNebulaEditor)
};
