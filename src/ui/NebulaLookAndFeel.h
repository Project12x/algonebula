#pragma once

#include "NebulaColours.h"
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>


/// Nebula design system LookAndFeel.
/// - Inter font for labels, JetBrains Mono for numeric readouts
/// - Gradient arc knobs with glow halos
/// - Dark palette from NebulaColours
class NebulaLookAndFeel : public juce::LookAndFeel_V4 {
public:
  NebulaLookAndFeel();
  ~NebulaLookAndFeel() override = default;

  //--- Fonts ---
  juce::Font getLabelFont(juce::Label &) override;
  juce::Font getTextButtonFont(juce::TextButton &, int buttonHeight) override;
  juce::Font getComboBoxFont(juce::ComboBox &) override;

  /// Font accessors for custom components
  juce::Font getInterFont(float height) const;
  juce::Font getMonoFont(float height) const;

  //--- Rotary Slider (NebulaDial) ---
  void drawRotarySlider(juce::Graphics &, int x, int y, int width, int height,
                        float sliderPos, float rotaryStartAngle,
                        float rotaryEndAngle, juce::Slider &) override;

  //--- Linear Slider ---
  void drawLinearSlider(juce::Graphics &, int x, int y, int width, int height,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        juce::Slider::SliderStyle, juce::Slider &) override;

  //--- Buttons ---
  void drawButtonBackground(juce::Graphics &, juce::Button &,
                            const juce::Colour &backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;

  //--- ComboBox ---
  void drawComboBox(juce::Graphics &, int width, int height, bool isButtonDown,
                    int buttonX, int buttonY, int buttonW, int buttonH,
                    juce::ComboBox &) override;

  //--- Label ---
  void drawLabel(juce::Graphics &, juce::Label &) override;

private:
  juce::Typeface::Ptr interRegular;
  juce::Typeface::Ptr interMedium;
  juce::Typeface::Ptr interSemiBold;
  juce::Typeface::Ptr monoRegular;
  juce::Typeface::Ptr monoLight;

  void loadFonts();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NebulaLookAndFeel)
};
