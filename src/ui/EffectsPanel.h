// EffectsPanel.h - Effects popout window with APVTS-attached knobs
// Hosts all DSP effect controls in a scrollable, sectioned layout.
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

    // Phaser
    setupKnob(phaserRateKnob, "Rate", "phaserRate");
    setupKnob(phaserDepthKnob, "Depth", "phaserDepth");
    setupKnob(phaserMixKnob, "Mix", "phaserMix");

    // Flanger
    setupKnob(flangerRateKnob, "Rate", "flangerRate");
    setupKnob(flangerDepthKnob, "Depth", "flangerDepth");
    setupKnob(flangerMixKnob, "Mix", "flangerMix");

    // Bitcrush
    setupKnob(bitcrushBitsKnob, "Bits", "bitcrushBits");
    setupKnob(bitcrushRateKnob, "Rate", "bitcrushRate");
    setupKnob(bitcrushMixKnob, "Mix", "bitcrushMix");

    // Tape Saturation
    setupKnob(tapeDriveKnob, "Drive", "tapeDrive");
    setupKnob(tapeToneKnob, "Tone", "tapeTone");
    setupKnob(tapeMixKnob, "Mix", "tapeMix");

    // Shimmer Reverb
    setupKnob(shimmerDecayKnob, "Decay", "shimmerDecay");
    setupKnob(shimmerAmountKnob, "Shimmer", "shimmerAmount");
    setupKnob(shimmerMixKnob, "Mix", "shimmerMix");

    // Ping Pong Delay
    setupKnob(pingPongTimeKnob, "Time", "pingPongTime");
    setupKnob(pingPongFeedbackKnob, "Fdbk", "pingPongFeedback");
    setupKnob(pingPongMixKnob, "Mix", "pingPongMix");
  }

  ~EffectsPanel() override { setLookAndFeel(nullptr); }

  void paint(juce::Graphics &g) override {
    g.fillAll(NebulaColours::bg_deepest);

    const int m = 12;

    auto drawSection = [&](const juce::String &text, int y) {
      g.setFont(nebulaLnF.getInterFont(11.0f));
      g.setColour(NebulaColours::accent1);
      g.drawText(text, m, y, 200, 14, juce::Justification::centredLeft);
    };

    auto drawDivider = [&](int y) {
      g.setColour(NebulaColours::divider);
      g.drawLine(static_cast<float>(m), static_cast<float>(y),
                 static_cast<float>(getWidth() - m), static_cast<float>(y),
                 1.0f);
    };

    // Section headers and dividers
    int y = 6;
    drawSection("STEREO", y);
    y += kSectionStereo;
    drawDivider(y - 4);
    drawSection("CHORUS", y);
    y += kSectionHeight;
    drawDivider(y - 4);
    drawSection("DELAY", y);
    y += kSectionHeight;
    drawDivider(y - 4);
    drawSection("REVERB", y);
    y += kSectionHeight;
    drawDivider(y - 4);
    drawSection("PHASER", y);
    y += kSectionHeight;
    drawDivider(y - 4);
    drawSection("FLANGER", y);
    y += kSectionHeight;
    drawDivider(y - 4);
    drawSection("BITCRUSH", y);
    y += kSectionHeight;
    drawDivider(y - 4);
    drawSection("TAPE SAT", y);
    y += kSectionHeight;
    drawDivider(y - 4);
    drawSection("SHIMMER", y);
    y += kSectionHeight;
    drawDivider(y - 4);
    drawSection("PING PONG", y);
  }

  void resized() override {
    const int m = 12;
    const int knobW = 56;
    const int knobH = 52;
    const int labelH = 14;

    int y = 22;

    // Stereo (1 knob)
    layoutKnob(stereoWidthKnob, m, y, knobW, knobH, labelH);
    y += kSectionStereo;

    // Chorus (3 knobs)
    layoutRow3(chorusRateKnob, chorusDepthKnob, chorusMixKnob, m, y, knobW,
               knobH, labelH);
    y += kSectionHeight;

    // Delay (3 knobs)
    layoutRow3(delayTimeKnob, delayFeedbackKnob, delayMixKnob, m, y, knobW,
               knobH, labelH);
    y += kSectionHeight;

    // Reverb (3 knobs)
    layoutRow3(reverbDecayKnob, reverbDampingKnob, reverbMixKnob, m, y, knobW,
               knobH, labelH);
    y += kSectionHeight;

    // Phaser (3 knobs)
    layoutRow3(phaserRateKnob, phaserDepthKnob, phaserMixKnob, m, y, knobW,
               knobH, labelH);
    y += kSectionHeight;

    // Flanger (3 knobs)
    layoutRow3(flangerRateKnob, flangerDepthKnob, flangerMixKnob, m, y, knobW,
               knobH, labelH);
    y += kSectionHeight;

    // Bitcrush (3 knobs)
    layoutRow3(bitcrushBitsKnob, bitcrushRateKnob, bitcrushMixKnob, m, y, knobW,
               knobH, labelH);
    y += kSectionHeight;

    // Tape Saturation (3 knobs)
    layoutRow3(tapeDriveKnob, tapeToneKnob, tapeMixKnob, m, y, knobW, knobH,
               labelH);
    y += kSectionHeight;

    // Shimmer Reverb (3 knobs)
    layoutRow3(shimmerDecayKnob, shimmerAmountKnob, shimmerMixKnob, m, y, knobW,
               knobH, labelH);
    y += kSectionHeight;

    // Ping Pong Delay (3 knobs)
    layoutRow3(pingPongTimeKnob, pingPongFeedbackKnob, pingPongMixKnob, m, y,
               knobW, knobH, labelH);
  }

private:
  static constexpr int kSectionStereo = 74; // Shorter section for 1 knob
  static constexpr int kSectionHeight = 80; // Standard section height

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

  void layoutRow3(Knob &k1, Knob &k2, Knob &k3, int m, int y, int knobW,
                  int knobH, int labelH) {
    layoutKnob(k1, m, y, knobW, knobH, labelH);
    layoutKnob(k2, m + knobW + 6, y, knobW, knobH, labelH);
    layoutKnob(k3, m + (knobW + 6) * 2, y, knobW, knobH, labelH);
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

  // Phaser
  Knob phaserRateKnob;
  Knob phaserDepthKnob;
  Knob phaserMixKnob;

  // Flanger
  Knob flangerRateKnob;
  Knob flangerDepthKnob;
  Knob flangerMixKnob;

  // Bitcrush
  Knob bitcrushBitsKnob;
  Knob bitcrushRateKnob;
  Knob bitcrushMixKnob;

  // Tape Saturation
  Knob tapeDriveKnob;
  Knob tapeToneKnob;
  Knob tapeMixKnob;

  // Shimmer Reverb
  Knob shimmerDecayKnob;
  Knob shimmerAmountKnob;
  Knob shimmerMixKnob;

  // Ping Pong Delay
  Knob pingPongTimeKnob;
  Knob pingPongFeedbackKnob;
  Knob pingPongMixKnob;

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
    // Taller window to accommodate all 10 effect sections
    centreWithSize(210, 860);
    setVisible(true);
    setAlwaysOnTop(false);
  }

  void closeButtonPressed() override { setVisible(false); }

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsWindow)
};
