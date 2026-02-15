#include "NebulaLookAndFeel.h"
#include "BinaryData.h"
#include <cmath>

//==============================================================================
NebulaLookAndFeel::NebulaLookAndFeel() {
  loadFonts();

  // Set default colours
  setColour(juce::ResizableWindow::backgroundColourId,
            NebulaColours::bg_deepest);
  setColour(juce::Label::textColourId, NebulaColours::text_normal);
  setColour(juce::Slider::textBoxTextColourId, NebulaColours::text_bright);
  setColour(juce::Slider::textBoxOutlineColourId,
            juce::Colours::transparentBlack);
  setColour(juce::ComboBox::backgroundColourId, NebulaColours::bg_surface);
  setColour(juce::ComboBox::textColourId, NebulaColours::text_bright);
  setColour(juce::ComboBox::outlineColourId, NebulaColours::divider);
  setColour(juce::TextButton::buttonColourId, NebulaColours::bg_surface);
  setColour(juce::TextButton::textColourOnId, NebulaColours::text_bright);
  setColour(juce::TextButton::textColourOffId, NebulaColours::text_normal);
  setColour(juce::PopupMenu::backgroundColourId, NebulaColours::bg_panel);
  setColour(juce::PopupMenu::textColourId, NebulaColours::text_normal);
  setColour(juce::PopupMenu::highlightedBackgroundColourId,
            NebulaColours::accent1_dim);
  setColour(juce::PopupMenu::highlightedTextColourId,
            NebulaColours::text_bright);
}

//==============================================================================
void NebulaLookAndFeel::loadFonts() {
  interRegular = juce::Typeface::createSystemTypefaceFor(
      BinaryData::InterRegular_ttf, BinaryData::InterRegular_ttfSize);
  interMedium = juce::Typeface::createSystemTypefaceFor(
      BinaryData::InterMedium_ttf, BinaryData::InterMedium_ttfSize);
  interSemiBold = juce::Typeface::createSystemTypefaceFor(
      BinaryData::InterSemiBold_ttf, BinaryData::InterSemiBold_ttfSize);
  monoRegular = juce::Typeface::createSystemTypefaceFor(
      BinaryData::JetBrainsMonoRegular_ttf,
      BinaryData::JetBrainsMonoRegular_ttfSize);
  monoLight = juce::Typeface::createSystemTypefaceFor(
      BinaryData::JetBrainsMonoLight_ttf,
      BinaryData::JetBrainsMonoLight_ttfSize);
}

//==============================================================================
juce::Font NebulaLookAndFeel::getInterFont(float height) const {
  return juce::Font(interRegular).withHeight(height);
}

juce::Font NebulaLookAndFeel::getMonoFont(float height) const {
  return juce::Font(monoRegular).withHeight(height);
}

juce::Font NebulaLookAndFeel::getLabelFont(juce::Label &) {
  return juce::Font(interRegular).withHeight(13.0f);
}

juce::Font NebulaLookAndFeel::getTextButtonFont(juce::TextButton &, int) {
  return juce::Font(interMedium).withHeight(13.0f);
}

juce::Font NebulaLookAndFeel::getComboBoxFont(juce::ComboBox &) {
  return juce::Font(interRegular).withHeight(13.0f);
}

//==============================================================================
void NebulaLookAndFeel::drawRotarySlider(juce::Graphics &g, int x, int y,
                                         int width, int height, float sliderPos,
                                         float rotaryStartAngle,
                                         float rotaryEndAngle,
                                         juce::Slider &slider) {
  const float radius = static_cast<float>(juce::jmin(width, height)) * 0.4f;
  const float centreX = static_cast<float>(x + width) * 0.5f;
  const float centreY = static_cast<float>(y + height) * 0.5f;
  const float arcWidth = 3.0f;

  const float angle =
      rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

  // --- Background track arc ---
  juce::Path bgArc;
  bgArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle,
                      rotaryEndAngle, true);
  g.setColour(NebulaColours::knob_track);
  g.strokePath(bgArc,
               juce::PathStrokeType(arcWidth, juce::PathStrokeType::curved,
                                    juce::PathStrokeType::rounded));

  // --- Filled gradient arc ---
  if (sliderPos > 0.001f) {
    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, radius, radius, 0.0f,
                           rotaryStartAngle, angle, true);

    // Gradient from accent1 to accent2 along the arc
    juce::ColourGradient gradient(NebulaColours::accent1, centreX - radius,
                                  centreY, NebulaColours::accent2,
                                  centreX + radius, centreY, false);
    g.setGradientFill(gradient);
    g.strokePath(valueArc,
                 juce::PathStrokeType(arcWidth, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));
  }

  // --- Glow halo at current position ---
  {
    const float glowRadius = 5.0f;
    const float thumbX =
        centreX + radius * std::cos(angle - juce::MathConstants<float>::halfPi);
    const float thumbY =
        centreY + radius * std::sin(angle - juce::MathConstants<float>::halfPi);

    juce::ColourGradient glow(NebulaColours::accent1_glow, thumbX, thumbY,
                              juce::Colours::transparentBlack,
                              thumbX + glowRadius * 2.0f, thumbY,
                              true); // radial
    g.setGradientFill(glow);
    g.fillEllipse(thumbX - glowRadius, thumbY - glowRadius, glowRadius * 2.0f,
                  glowRadius * 2.0f);

    // Solid thumb dot
    g.setColour(NebulaColours::text_bright);
    g.fillEllipse(thumbX - 2.5f, thumbY - 2.5f, 5.0f, 5.0f);
  }

  // --- Center dot ---
  g.setColour(NebulaColours::bg_surface);
  g.fillEllipse(centreX - 3.0f, centreY - 3.0f, 6.0f, 6.0f);

  // --- Value text below knob ---
  if (slider.isMouseOverOrDragging()) {
    g.setColour(NebulaColours::text_bright);
    g.setFont(juce::Font(monoRegular).withHeight(11.0f));
    g.drawText(slider.getTextFromValue(slider.getValue()), x, y + height - 14,
               width, 14, juce::Justification::centred, false);
  }
}

//==============================================================================
void NebulaLookAndFeel::drawLinearSlider(juce::Graphics &g, int x, int y,
                                         int width, int height, float sliderPos,
                                         float minSliderPos, float maxSliderPos,
                                         juce::Slider::SliderStyle style,
                                         juce::Slider &slider) {
  // Horizontal slider with gradient fill
  if (style == juce::Slider::LinearHorizontal) {
    const float trackHeight = 4.0f;
    const float trackY =
        static_cast<float>(y + height) * 0.5f - trackHeight * 0.5f;

    // Background track
    g.setColour(NebulaColours::knob_track);
    g.fillRoundedRectangle(static_cast<float>(x), trackY,
                           static_cast<float>(width), trackHeight, 2.0f);

    // Filled portion
    const float fillWidth = sliderPos - static_cast<float>(x);
    if (fillWidth > 0) {
      juce::ColourGradient gradient(
          NebulaColours::accent1, static_cast<float>(x), trackY,
          NebulaColours::accent2, static_cast<float>(x + width), trackY, false);
      g.setGradientFill(gradient);
      g.fillRoundedRectangle(static_cast<float>(x), trackY, fillWidth,
                             trackHeight, 2.0f);
    }

    // Thumb
    g.setColour(NebulaColours::text_bright);
    g.fillEllipse(sliderPos - 5.0f, trackY - 3.0f, 10.0f, 10.0f);
  } else {
    LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                     minSliderPos, maxSliderPos, style, slider);
  }
}

//==============================================================================
void NebulaLookAndFeel::drawButtonBackground(juce::Graphics &g,
                                             juce::Button &button,
                                             const juce::Colour &,
                                             bool isHighlighted, bool isDown) {
  auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);

  juce::Colour bg = isDown          ? NebulaColours::accent1_dim
                    : isHighlighted ? NebulaColours::bg_hover
                                    : NebulaColours::bg_surface;

  g.setColour(bg);
  g.fillRoundedRectangle(bounds, 4.0f);

  g.setColour(NebulaColours::divider);
  g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

//==============================================================================
void NebulaLookAndFeel::drawComboBox(juce::Graphics &g, int width, int height,
                                     bool isButtonDown, int, int, int, int,
                                     juce::ComboBox &box) {
  auto bounds = box.getLocalBounds().toFloat();

  g.setColour(isButtonDown ? NebulaColours::bg_hover
                           : NebulaColours::bg_surface);
  g.fillRoundedRectangle(bounds, 4.0f);

  g.setColour(NebulaColours::divider);
  g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

  // Down arrow
  juce::Path arrow;
  const float arrowX = static_cast<float>(width) - 16.0f;
  const float arrowY = static_cast<float>(height) * 0.5f - 2.0f;
  arrow.addTriangle(arrowX, arrowY, arrowX + 8.0f, arrowY, arrowX + 4.0f,
                    arrowY + 5.0f);
  g.setColour(NebulaColours::text_dim);
  g.fillPath(arrow);
}

//==============================================================================
void NebulaLookAndFeel::drawLabel(juce::Graphics &g, juce::Label &label) {
  g.fillAll(label.findColour(juce::Label::backgroundColourId));

  if (!label.isBeingEdited()) {
    const auto textColour = label.findColour(juce::Label::textColourId);
    g.setColour(textColour.isTransparent() ? NebulaColours::text_normal
                                           : textColour);
    g.setFont(getLabelFont(label));

    auto textArea =
        label.getBorderSize().subtractedFrom(label.getLocalBounds());
    g.drawFittedText(
        label.getText(), textArea, label.getJustificationType(),
        juce::jmax(1, static_cast<int>(textArea.getHeight() / 12.0f)),
        label.getMinimumHorizontalScale());
  }
}
