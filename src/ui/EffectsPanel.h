#pragma once

#include "../PluginProcessor.h"
#include "NebulaColours.h"
#include "NebulaLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>

// Self-contained effects panel with APVTS-attached knobs.
// Hosted inside EffectsWindow (non-modal DocumentWindow).
class EffectsPanel : public juce::Component {
public:
  explicit EffectsPanel(AlgoNebulaProcessor &proc, NebulaLookAndFeel &lnf)
      : processor(proc), nebulaLnF(lnf) {
    setLookAndFeel(&nebulaLnF);

    // Stereo
    setupKnob(stereoWidthKnob, "Width", "stereoWidth");

    // Chorus
    setupKnob(chorusRateKnob, "Rate", "chorusRate");
    setupKnob(chorusDepthKnob, "Depth", "chorusDepth");
    setupKnob(chorusMixKnob, "Mix", "chorusMix");

    // Delay
    setupKnob(delayTimeKnob, "Time", "delayTime");
    setupKnob(delayFeedbackKnob, "Fdbk", "delayFeedback");
    setupKnob(delayMixKnob, "Mix", "delayMix");

    // Reverb
    setupKnob(reverbDecayKnob, "Decay", "reverbDecay");
    setupKnob(reverbDampingKnob, "Damp", "reverbDamping");
    setupKnob(reverbMixKnob, "Mix", "reverbMix");
  }

  ~EffectsPanel() override { setLookAndFeel(nullptr); }

  void paint(juce::Graphics &g) override {
    // Background
    g.fillAll(NebulaColours::bg_deepest);

    auto drawSection = [&](const juce::String &text, int x, int y) {
      g.setFont(nebulaLnF.getInterFont(11.0f));
      g.setColour(NebulaColours::accent1);
      g.drawText(text, x, y, 200, 14, juce::Justification::centredLeft);
    };

    const int m = 12;
    drawSection("STEREO", m, 6);
    drawSection("CHORUS", m, 80);
    drawSection("DELAY", m, 160);
    drawSection("REVERB", m, 240);

    // Dividers
    g.setColour(NebulaColours::divider);
    g.drawLine(static_cast<float>(m), 76.0f, static_cast<float>(getWidth() - m),
               76.0f, 1.0f);
    g.drawLine(static_cast<float>(m), 156.0f,
               static_cast<float>(getWidth() - m), 156.0f, 1.0f);
    g.drawLine(static_cast<float>(m), 236.0f,
               static_cast<float>(getWidth() - m), 236.0f, 1.0f);
  }

  void resized() override {
    const int m = 12;
    const int knobW = 56;
    const int knobH = 52;
    const int labelH = 14;
    const int rowH = knobH + labelH + 2;

    // Stereo (1 knob, centered)
    layoutKnob(stereoWidthKnob, m, 22, knobW, knobH, labelH);

    // Chorus (3 knobs)
    layoutKnob(chorusRateKnob, m, 96, knobW, knobH, labelH);
    layoutKnob(chorusDepthKnob, m + knobW + 6, 96, knobW, knobH, labelH);
    layoutKnob(chorusMixKnob, m + (knobW + 6) * 2, 96, knobW, knobH, labelH);

    // Delay (3 knobs)
    layoutKnob(delayTimeKnob, m, 176, knobW, knobH, labelH);
    layoutKnob(delayFeedbackKnob, m + knobW + 6, 176, knobW, knobH, labelH);
    layoutKnob(delayMixKnob, m + (knobW + 6) * 2, 176, knobW, knobH, labelH);

    // Reverb (3 knobs)
    layoutKnob(reverbDecayKnob, m, 256, knobW, knobH, labelH);
    layoutKnob(reverbDampingKnob, m + knobW + 6, 256, knobW, knobH, labelH);
    layoutKnob(reverbMixKnob, m + (knobW + 6) * 2, 256, knobW, knobH, labelH);
  }

private:
  struct Knob {
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        attach;
  };

  void setupKnob(Knob &k, const juce::String &labelText,
                 const juce::String &paramID) {
    k.slider.setSliderStyle(juce::Slider::Rotary);
    k.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    k.slider.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(k.slider);

    k.label.setText(labelText, juce::dontSendNotification);
    k.label.setJustificationType(juce::Justification::centred);
    k.label.setColour(juce::Label::textColourId, NebulaColours::text_normal);
    addAndMakeVisible(k.label);

    k.attach =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getAPVTS(), paramID, k.slider);
  }

  void layoutKnob(Knob &k, int x, int y, int w, int h, int lh) {
    k.slider.setBounds(x, y, w, h);
    k.label.setBounds(x, y + h + 2, w, lh);
  }

  AlgoNebulaProcessor &processor;
  NebulaLookAndFeel &nebulaLnF;

  // Stereo
  Knob stereoWidthKnob;

  // Chorus
  Knob chorusRateKnob;
  Knob chorusDepthKnob;
  Knob chorusMixKnob;

  // Delay
  Knob delayTimeKnob;
  Knob delayFeedbackKnob;
  Knob delayMixKnob;

  // Reverb
  Knob reverbDecayKnob;
  Knob reverbDampingKnob;
  Knob reverbMixKnob;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsPanel)
};

// Non-modal floating window that hosts the EffectsPanel.
// Does NOT prevent interaction with the main editor.
class EffectsWindow : public juce::DocumentWindow {
public:
  EffectsWindow(AlgoNebulaProcessor &proc, NebulaLookAndFeel &lnf)
      : DocumentWindow("Effects", NebulaColours::bg_deepest,
                       DocumentWindow::closeButton) {
    setUsingNativeTitleBar(false);
    setContentOwned(new EffectsPanel(proc, lnf), false);
    setResizable(false, false);
    centreWithSize(210, 330);
    setVisible(true);
    setAlwaysOnTop(false);
  }

  void closeButtonPressed() override { setVisible(false); }

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsWindow)
};
