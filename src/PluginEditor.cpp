#include "PluginEditor.h"

//==============================================================================
AlgoNebulaEditor::AlgoNebulaEditor(AlgoNebulaProcessor &p)
    : AudioProcessorEditor(p), processor(p) {
  setLookAndFeel(&nebulaLnF);

  // --- Resizable with aspect ratio ---
  setResizable(true, true);
  setResizeLimits(900, 600, 1920, 1280);
  getConstrainer()->setFixedAspectRatio(3.0 / 2.0);
  setSize(900, 600);

  // --- Master Volume (rotary) ---
  masterVolumeSlider.setSliderStyle(juce::Slider::Rotary);
  masterVolumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  masterVolumeSlider.setPopupDisplayEnabled(true, true, this);
  addAndMakeVisible(masterVolumeSlider);

  masterVolumeLabel.setText("Volume", juce::dontSendNotification);
  masterVolumeLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(masterVolumeLabel);

  masterVolumeAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor.getAPVTS(), "masterVolume", masterVolumeSlider);

  // --- Algorithm Selector ---
  auto *algoParam = dynamic_cast<juce::AudioParameterChoice *>(
      processor.getAPVTS().getParameter("algorithm"));
  if (algoParam != nullptr)
    algorithmSelector.addItemList(algoParam->choices, 1);
  addAndMakeVisible(algorithmSelector);

  algorithmLabel.setText("Algorithm", juce::dontSendNotification);
  algorithmLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(algorithmLabel);

  algorithmAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          processor.getAPVTS(), "algorithm", algorithmSelector);

  // --- Scale Selector ---
  auto *scaleParam = dynamic_cast<juce::AudioParameterChoice *>(
      processor.getAPVTS().getParameter("scale"));
  if (scaleParam != nullptr)
    scaleSelector.addItemList(scaleParam->choices, 1);
  addAndMakeVisible(scaleSelector);

  scaleLabel.setText("Scale", juce::dontSendNotification);
  scaleLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(scaleLabel);

  scaleAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          processor.getAPVTS(), "scale", scaleSelector);

  // --- Key Selector ---
  auto *keyParam = dynamic_cast<juce::AudioParameterChoice *>(
      processor.getAPVTS().getParameter("key"));
  if (keyParam != nullptr)
    keySelector.addItemList(keyParam->choices, 1);
  addAndMakeVisible(keySelector);

  keyLabel.setText("Key", juce::dontSendNotification);
  keyLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(keyLabel);

  keyAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          processor.getAPVTS(), "key", keySelector);

  // --- Waveshape Selector ---
  auto *waveParam = dynamic_cast<juce::AudioParameterChoice *>(
      processor.getAPVTS().getParameter("waveshape"));
  if (waveParam != nullptr)
    waveshapeSelector.addItemList(waveParam->choices, 1);
  addAndMakeVisible(waveshapeSelector);

  waveshapeLabel.setText("Wave", juce::dontSendNotification);
  waveshapeLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(waveshapeLabel);

  waveshapeAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          processor.getAPVTS(), "waveshape", waveshapeSelector);

  // --- CPU Meter ---
  cpuMeterLabel.setFont(nebulaLnF.getMonoFont(12.0f));
  cpuMeterLabel.setColour(juce::Label::textColourId, NebulaColours::text_dim);
  cpuMeterLabel.setJustificationType(juce::Justification::centredRight);
  cpuMeterLabel.setText("CPU: 0.0%", juce::dontSendNotification);
  addAndMakeVisible(cpuMeterLabel);

  // Start CPU meter refresh (10 Hz)
  startTimerHz(10);
}

AlgoNebulaEditor::~AlgoNebulaEditor() { setLookAndFeel(nullptr); }

//==============================================================================
void AlgoNebulaEditor::paint(juce::Graphics &g) {
  // Dark background with subtle gradient
  juce::ColourGradient bgGradient(NebulaColours::bg_deepest, 0.0f, 0.0f,
                                  NebulaColours::bg_panel, 0.0f,
                                  static_cast<float>(getHeight()), false);
  g.setGradientFill(bgGradient);
  g.fillAll();

  // Title
  g.setColour(NebulaColours::text_bright);
  g.setFont(nebulaLnF.getInterFont(24.0f));
  g.drawText("Algo Nebula", 20, 10, 300, 40, juce::Justification::centredLeft);

  // Version badge
  g.setFont(nebulaLnF.getMonoFont(10.0f));
  g.setColour(NebulaColours::text_dim);
  g.drawText("v0.1.0", 20, 42, 100, 16, juce::Justification::centredLeft);

  // Divider below header
  g.setColour(NebulaColours::divider);
  g.drawLine(20.0f, 65.0f, static_cast<float>(getWidth() - 20), 65.0f, 1.0f);
}

void AlgoNebulaEditor::resized() {
  auto area = getLocalBounds().reduced(20);
  area.removeFromTop(70); // Below header

  // Top row: selectors
  auto topRow = area.removeFromTop(60);
  const int selectorWidth = (topRow.getWidth() - 30) / 4;

  auto algoArea = topRow.removeFromLeft(selectorWidth);
  topRow.removeFromLeft(10);
  auto scaleArea = topRow.removeFromLeft(selectorWidth);
  topRow.removeFromLeft(10);
  auto keyArea = topRow.removeFromLeft(selectorWidth);
  topRow.removeFromLeft(10);
  auto waveArea = topRow;

  algorithmLabel.setBounds(algoArea.removeFromTop(18));
  algorithmSelector.setBounds(algoArea.reduced(0, 4));

  scaleLabel.setBounds(scaleArea.removeFromTop(18));
  scaleSelector.setBounds(scaleArea.reduced(0, 4));

  keyLabel.setBounds(keyArea.removeFromTop(18));
  keySelector.setBounds(keyArea.reduced(0, 4));

  waveshapeLabel.setBounds(waveArea.removeFromTop(18));
  waveshapeSelector.setBounds(waveArea.reduced(0, 4));

  area.removeFromTop(20);

  // Volume knob (bottom-right)
  const int knobSize = 80;
  auto bottomRight =
      area.removeFromBottom(knobSize + 20).removeFromRight(knobSize + 20);
  masterVolumeLabel.setBounds(bottomRight.removeFromBottom(18));
  masterVolumeSlider.setBounds(bottomRight);

  // CPU meter (bottom-left)
  cpuMeterLabel.setBounds(getWidth() - 120, getHeight() - 24, 100, 16);
}

//==============================================================================
void AlgoNebulaEditor::timerCallback() {
  float cpu = processor.getCpuLoadPercent();
  cpuMeterLabel.setText(juce::String("CPU: ") + juce::String(cpu, 1) + "%",
                        juce::dontSendNotification);

  // Color based on load
  if (cpu > 80.0f)
    cpuMeterLabel.setColour(juce::Label::textColourId, NebulaColours::danger);
  else if (cpu > 50.0f)
    cpuMeterLabel.setColour(juce::Label::textColourId, NebulaColours::warning);
  else
    cpuMeterLabel.setColour(juce::Label::textColourId, NebulaColours::text_dim);
}
