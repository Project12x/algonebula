// EffectsPanel.h - Effects popout window with APVTS-attached knobs and toggles
// Hosts all DSP effect controls + LFO modulation matrix + trigger budget.
#pragma once

#include "../PluginProcessor.h"
#include "NebulaColours.h"
#include "NebulaLookAndFeel.h"
#include <juce_gui_basics/juce_gui_basics.h>

// Self-contained effects panel with APVTS-attached knobs and toggles.
// Hosted inside EffectsWindow (non-modal DocumentWindow).
class EffectsPanel : public juce::Component {
public:
  explicit EffectsPanel(AlgoNebulaProcessor &proc, NebulaLookAndFeel &lnf)
      : processor(proc), nebulaLnF(lnf) {
    setLookAndFeel(&nebulaLnF);

    // Stereo
    setupKnob(stereoWidthKnob, "Width", "stereoWidth");

    // Effect sections with toggles
    setupToggle(chorusToggle, "chorusOn");
    setupKnob(chorusRateKnob, "Rate", "chorusRate");
    setupKnob(chorusDepthKnob, "Depth", "chorusDepth");
    setupKnob(chorusMixKnob, "Mix", "chorusMix");

    setupToggle(delayToggle, "delayOn");
    setupKnob(delayTimeKnob, "Time", "delayTime");
    setupKnob(delayFeedbackKnob, "Fdbk", "delayFeedback");
    setupKnob(delayMixKnob, "Mix", "delayMix");

    setupToggle(reverbToggle, "reverbOn");
    setupKnob(reverbDecayKnob, "Decay", "reverbDecay");
    setupKnob(reverbDampingKnob, "Damp", "reverbDamping");
    setupKnob(reverbMixKnob, "Mix", "reverbMix");

    setupToggle(phaserToggle, "phaserOn");
    setupKnob(phaserRateKnob, "Rate", "phaserRate");
    setupKnob(phaserDepthKnob, "Depth", "phaserDepth");
    setupKnob(phaserMixKnob, "Mix", "phaserMix");

    setupToggle(flangerToggle, "flangerOn");
    setupKnob(flangerRateKnob, "Rate", "flangerRate");
    setupKnob(flangerDepthKnob, "Depth", "flangerDepth");
    setupKnob(flangerMixKnob, "Mix", "flangerMix");

    setupToggle(bitcrushToggle, "bitcrushOn");
    setupKnob(bitcrushBitsKnob, "Bits", "bitcrushBits");
    setupKnob(bitcrushRateKnob, "Rate", "bitcrushRate");
    setupKnob(bitcrushMixKnob, "Mix", "bitcrushMix");

    setupToggle(tapeToggle, "tapeOn");
    setupKnob(tapeDriveKnob, "Drive", "tapeDrive");
    setupKnob(tapeToneKnob, "Tone", "tapeTone");
    setupKnob(tapeMixKnob, "Mix", "tapeMix");

    setupToggle(shimmerToggle, "shimmerOn");
    setupKnob(shimmerDecayKnob, "Decay", "shimmerDecay");
    setupKnob(shimmerAmountKnob, "Shimmer", "shimmerAmount");
    setupKnob(shimmerMixKnob, "Mix", "shimmerMix");

    setupToggle(pingPongToggle, "pingPongOn");
    setupKnob(pingPongTimeKnob, "Time", "pingPongTime");
    setupKnob(pingPongFeedbackKnob, "Fdbk", "pingPongFeedback");
    setupKnob(pingPongMixKnob, "Mix", "pingPongMix");

    // Trigger Budget
    setupKnob(triggerBudgetKnob, "Budget", "triggerBudget");

    // LFO Modulation
    setupKnob(lfo1ShapeKnob, "Shape", "lfo1Shape");
    setupKnob(lfo1RateKnob, "Rate", "lfo1Rate");
    setupKnob(lfo1AmountKnob, "Amt", "lfo1Amount");
    setupKnob(lfo1DestKnob, "Dest", "lfo1Dest");

    setupKnob(lfo2ShapeKnob, "Shape", "lfo2Shape");
    setupKnob(lfo2RateKnob, "Rate", "lfo2Rate");
    setupKnob(lfo2AmountKnob, "Amt", "lfo2Amount");
    setupKnob(lfo2DestKnob, "Dest", "lfo2Dest");
  }

  ~EffectsPanel() override { setLookAndFeel(nullptr); }

  void paint(juce::Graphics &g) override {
    g.fillAll(NebulaColours::bg_deepest);

    const int m = 12;
    const int toggleW = 16;

    auto drawSection = [&](const juce::String &text, int y,
                           bool hasToggle = true) {
      g.setFont(nebulaLnF.getInterFont(11.0f));
      g.setColour(NebulaColours::accent1);
      int textX = hasToggle ? m + toggleW + 4 : m;
      g.drawText(text, textX, y, 200, 14, juce::Justification::centredLeft);
    };

    auto drawDivider = [&](int y) {
      g.setColour(NebulaColours::divider);
      g.drawLine(static_cast<float>(m), static_cast<float>(y),
                 static_cast<float>(getWidth() - m), static_cast<float>(y),
                 1.0f);
    };

    int y = 6;
    drawSection("STEREO", y, false);
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
    y += kSectionHeight;
    drawDivider(y - 4);
    drawSection("TRIGGER BUDGET", y, false);
    y += kSectionStereo;
    drawDivider(y - 4);
    drawSection("MOD LFO 1", y, false);
    y += kSectionHeight;
    drawDivider(y - 4);
    drawSection("MOD LFO 2", y, false);
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

    // Chorus (toggle + 3 knobs)
    layoutToggle(chorusToggle, m, y - 16);
    layoutRow3(chorusRateKnob, chorusDepthKnob, chorusMixKnob, m, y, knobW,
               knobH, labelH);
    y += kSectionHeight;

    // Delay
    layoutToggle(delayToggle, m, y - 16);
    layoutRow3(delayTimeKnob, delayFeedbackKnob, delayMixKnob, m, y, knobW,
               knobH, labelH);
    y += kSectionHeight;

    // Reverb
    layoutToggle(reverbToggle, m, y - 16);
    layoutRow3(reverbDecayKnob, reverbDampingKnob, reverbMixKnob, m, y, knobW,
               knobH, labelH);
    y += kSectionHeight;

    // Phaser
    layoutToggle(phaserToggle, m, y - 16);
    layoutRow3(phaserRateKnob, phaserDepthKnob, phaserMixKnob, m, y, knobW,
               knobH, labelH);
    y += kSectionHeight;

    // Flanger
    layoutToggle(flangerToggle, m, y - 16);
    layoutRow3(flangerRateKnob, flangerDepthKnob, flangerMixKnob, m, y, knobW,
               knobH, labelH);
    y += kSectionHeight;

    // Bitcrush
    layoutToggle(bitcrushToggle, m, y - 16);
    layoutRow3(bitcrushBitsKnob, bitcrushRateKnob, bitcrushMixKnob, m, y, knobW,
               knobH, labelH);
    y += kSectionHeight;

    // Tape Saturation
    layoutToggle(tapeToggle, m, y - 16);
    layoutRow3(tapeDriveKnob, tapeToneKnob, tapeMixKnob, m, y, knobW, knobH,
               labelH);
    y += kSectionHeight;

    // Shimmer Reverb
    layoutToggle(shimmerToggle, m, y - 16);
    layoutRow3(shimmerDecayKnob, shimmerAmountKnob, shimmerMixKnob, m, y, knobW,
               knobH, labelH);
    y += kSectionHeight;

    // Ping Pong Delay
    layoutToggle(pingPongToggle, m, y - 16);
    layoutRow3(pingPongTimeKnob, pingPongFeedbackKnob, pingPongMixKnob, m, y,
               knobW, knobH, labelH);
    y += kSectionHeight;

    // Trigger Budget (1 knob)
    layoutKnob(triggerBudgetKnob, m, y, knobW, knobH, labelH);
    y += kSectionStereo;

    // LFO 1 (4 knobs)
    layoutRow4(lfo1ShapeKnob, lfo1RateKnob, lfo1AmountKnob, lfo1DestKnob, m, y,
               knobW - 6, knobH, labelH);
    y += kSectionHeight;

    // LFO 2 (4 knobs)
    layoutRow4(lfo2ShapeKnob, lfo2RateKnob, lfo2AmountKnob, lfo2DestKnob, m, y,
               knobW - 6, knobH, labelH);
  }

private:
  static constexpr int kSectionStereo = 74;
  static constexpr int kSectionHeight = 80;

  struct Knob {
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        attach;
  };

  struct Toggle {
    juce::ToggleButton button;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
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

  void setupToggle(Toggle &t, const juce::String &paramID) {
    t.button.setButtonText("");
    t.button.setColour(juce::ToggleButton::tickColourId,
                       NebulaColours::accent1);
    addAndMakeVisible(t.button);
    t.attach =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor.getAPVTS(), paramID, t.button);
  }

  void layoutKnob(Knob &k, int x, int y, int w, int h, int lh) {
    k.slider.setBounds(x, y, w, h);
    k.label.setBounds(x, y + h + 2, w, lh);
  }

  void layoutToggle(Toggle &t, int x, int y) {
    t.button.setBounds(x, y, 16, 16);
  }

  void layoutRow3(Knob &k1, Knob &k2, Knob &k3, int m, int y, int knobW,
                  int knobH, int labelH) {
    layoutKnob(k1, m, y, knobW, knobH, labelH);
    layoutKnob(k2, m + knobW + 6, y, knobW, knobH, labelH);
    layoutKnob(k3, m + (knobW + 6) * 2, y, knobW, knobH, labelH);
  }

  void layoutRow4(Knob &k1, Knob &k2, Knob &k3, Knob &k4, int m, int y,
                  int knobW, int knobH, int labelH) {
    layoutKnob(k1, m, y, knobW, knobH, labelH);
    layoutKnob(k2, m + knobW + 4, y, knobW, knobH, labelH);
    layoutKnob(k3, m + (knobW + 4) * 2, y, knobW, knobH, labelH);
    layoutKnob(k4, m + (knobW + 4) * 3, y, knobW, knobH, labelH);
  }

  AlgoNebulaProcessor &processor;
  NebulaLookAndFeel &nebulaLnF;

  // Stereo
  Knob stereoWidthKnob;

  // Effect toggles
  Toggle chorusToggle, delayToggle, reverbToggle, phaserToggle, flangerToggle,
      bitcrushToggle, tapeToggle, shimmerToggle, pingPongToggle;

  // Chorus
  Knob chorusRateKnob, chorusDepthKnob, chorusMixKnob;

  // Delay
  Knob delayTimeKnob, delayFeedbackKnob, delayMixKnob;

  // Reverb
  Knob reverbDecayKnob, reverbDampingKnob, reverbMixKnob;

  // Phaser
  Knob phaserRateKnob, phaserDepthKnob, phaserMixKnob;

  // Flanger
  Knob flangerRateKnob, flangerDepthKnob, flangerMixKnob;

  // Bitcrush
  Knob bitcrushBitsKnob, bitcrushRateKnob, bitcrushMixKnob;

  // Tape Saturation
  Knob tapeDriveKnob, tapeToneKnob, tapeMixKnob;

  // Shimmer Reverb
  Knob shimmerDecayKnob, shimmerAmountKnob, shimmerMixKnob;

  // Ping Pong Delay
  Knob pingPongTimeKnob, pingPongFeedbackKnob, pingPongMixKnob;

  // Trigger Budget
  Knob triggerBudgetKnob;

  // LFO Modulation
  Knob lfo1ShapeKnob, lfo1RateKnob, lfo1AmountKnob, lfo1DestKnob;
  Knob lfo2ShapeKnob, lfo2RateKnob, lfo2AmountKnob, lfo2DestKnob;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsPanel)
};

// Non-modal floating window that hosts the EffectsPanel.
class EffectsWindow : public juce::DocumentWindow {
public:
  EffectsWindow(AlgoNebulaProcessor &proc, NebulaLookAndFeel &lnf)
      : DocumentWindow("Effects", NebulaColours::bg_deepest,
                       DocumentWindow::closeButton) {
    setUsingNativeTitleBar(false);
    setContentOwned(new EffectsPanel(proc, lnf), false);
    setResizable(false, false);
    // Taller window: 10 effects + trigger budget + 2 LFOs
    centreWithSize(230, 1120);
    setVisible(true);
    setAlwaysOnTop(false);
  }

  void closeButtonPressed() override { setVisible(false); }

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsWindow)
};
