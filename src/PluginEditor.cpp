#include "PluginEditor.h"

//==============================================================================
AlgoNebulaEditor::AlgoNebulaEditor(AlgoNebulaProcessor &p)
    : AudioProcessorEditor(p), processor(p), gridComponent(p),
      midiKeyboard(p.getKeyboardState(),
                   juce::MidiKeyboardComponent::horizontalKeyboard) {
  setLookAndFeel(&nebulaLnF);

  // --- Resizable ---
  setResizable(true, true);
  setResizeLimits(1000, 780, 1920, 1400);
  getConstrainer()->setFixedAspectRatio(0.0); // freeform resize
  setSize(1000, 780);

  // --- Grid ---
  addAndMakeVisible(gridComponent);

  // --- Preset selector ---
  factoryPresets = getFactoryPresets();
  presetLabel.setText("Preset", juce::dontSendNotification);
  presetLabel.setFont(nebulaLnF.getMonoFont(10.0f));
  presetLabel.setColour(juce::Label::textColourId, NebulaColours::text_dim);
  presetLabel.setJustificationType(juce::Justification::centredRight);
  addAndMakeVisible(presetLabel);

  presetCombo.setTextWhenNothingSelected("-- Preset --");
  for (int i = 0; i < static_cast<int>(factoryPresets.size()); ++i)
    presetCombo.addItem(factoryPresets[static_cast<size_t>(i)].name, i + 1);
  presetCombo.onChange = [this]() {
    int idx = presetCombo.getSelectedId() - 1;
    if (idx >= 0 && idx < static_cast<int>(factoryPresets.size()))
      factoryPresets[static_cast<size_t>(idx)].apply(processor.getAPVTS());
  };
  presetCombo.setTooltip("Factory preset: load a curated parameter set");
  addAndMakeVisible(presetCombo);

  // --- Top selectors ---
  setupCombo(algorithmCombo, "Algorithm", "algorithm");
  algorithmCombo.combo.setTooltip(
      "Cellular automaton algorithm driving the grid");
  setupCombo(scaleCombo, "Scale", "scale");
  scaleCombo.combo.setTooltip("Musical scale for pitch quantization");
  setupCombo(keyCombo, "Key", "key");
  keyCombo.combo.setTooltip("Root key of the scale");
  setupCombo(waveshapeCombo, "Wave", "waveshape");
  waveshapeCombo.combo.setTooltip("Oscillator waveform shape");

  // --- Clock ---
  setupKnob(bpmKnob, "BPM", "bpm");
  bpmKnob.slider.setTooltip("Tempo in beats per minute (40-300)");
  setupCombo(clockDivCombo, "Clock", "clockDiv");
  clockDivCombo.combo.setTooltip("Clock division: how often the grid steps");
  setupKnob(swingKnob, "Swing", "swing");
  swingKnob.slider.setTooltip(
      "Swing feel: offsets every other step (0% = straight)");

  // --- Envelope ---
  setupKnob(attackKnob, "Atk", "attack");
  attackKnob.slider.setTooltip("Attack time: how quickly notes fade in");
  setupKnob(holdKnob, "Hold", "hold");
  holdKnob.slider.setTooltip("Hold time: sustain at full level before decay");
  setupKnob(decayKnob, "Dcy", "decay");
  decayKnob.slider.setTooltip(
      "Decay time: fade from full level to sustain level");
  setupKnob(sustainKnob, "Sus", "sustain");
  sustainKnob.slider.setTooltip("Sustain level: volume while note is held");
  setupKnob(releaseKnob, "Rel", "release");
  releaseKnob.slider.setTooltip("Release time: fade out after note ends");

  // --- Filter ---
  setupKnob(filterCutoffKnob, "Cutoff", "filterCutoff");
  filterCutoffKnob.slider.setTooltip(
      "Filter cutoff frequency (20 Hz - 20 kHz)");
  setupKnob(filterResKnob, "Reso", "filterRes");
  filterResKnob.slider.setTooltip(
      "Filter resonance: emphasis at cutoff frequency");
  setupCombo(filterModeCombo, "Filter", "filterMode");
  filterModeCombo.combo.setTooltip(
      "Filter type: Low Pass / High Pass / Band Pass / Notch");

  // --- Mix ---
  setupKnob(noiseLevelKnob, "Noise", "noiseLevel");
  noiseLevelKnob.slider.setTooltip("White noise layer level");
  setupKnob(waveshapeSpreadKnob, "WSpread", "waveshapeSpread");
  waveshapeSpreadKnob.slider.setTooltip(
      "Waveshape spread: 0 = all voices use selected wave, 1 = cycle shapes");
  setupKnob(subLevelKnob, "Sub", "subLevel");
  subLevelKnob.slider.setTooltip(
      "Sub-oscillator level (sine, tracks lowest voice)");
  setupCombo(subOctaveCombo, "Sub Oct", "subOctave");
  subOctaveCombo.combo.setTooltip(
      "Sub-oscillator octave: -1 or -2 octaves below");

  // --- Tuning ---
  setupCombo(tuningCombo, "Tuning", "tuning");
  tuningCombo.combo.setTooltip(
      "Tuning system: 12-TET, Just Intonation, Pythagorean");
  setupKnob(refPitchKnob, "A4 Hz", "refPitch");
  refPitchKnob.slider.setTooltip(
      "Reference pitch for A4 (420-460 Hz, standard = 440)");

  // --- Ambient ---
  setupKnob(droneSustainKnob, "Drone", "droneSustain");
  droneSustainKnob.slider.setTooltip(
      "Drone sustain: how long voices ring after cell death");
  setupKnob(noteProbKnob, "Prob", "noteProbability");
  noteProbKnob.slider.setTooltip("Note probability: chance each cell triggers "
                                 "a note (0 = silent, 1 = always)");
  setupKnob(gateTimeKnob, "Gate", "gateTime");
  gateTimeKnob.slider.setTooltip("Gate time: note duration as fraction of step "
                                 "(1 = legato, 0.5 = staccato)");

  // --- Humanize ---
  setupKnob(strumSpreadKnob, "Strum", "strumSpread");
  strumSpreadKnob.slider.setTooltip(
      "Strum spread: staggers note onsets across columns (0-50 ms)");
  setupKnob(melodicInertiaKnob, "Inertia", "melodicInertia");
  melodicInertiaKnob.slider.setTooltip(
      "Melodic inertia: chance to repeat the last pitch instead of a new one");
  setupKnob(roundRobinKnob, "RndRbn", "roundRobin");
  roundRobinKnob.slider.setTooltip("Round-robin variation depth");
  setupKnob(velHumanizeKnob, "VelHum", "velocityHumanize");
  velHumanizeKnob.slider.setTooltip(
      "Velocity humanize: random velocity offset for natural dynamics");

  // --- Global ---
  setupKnob(masterVolumeKnob, "Volume", "masterVolume");
  masterVolumeKnob.slider.setTooltip("Master output volume (0-200%)");
  setupKnob(voiceCountKnob, "Voices", "voiceCount");
  voiceCountKnob.slider.setTooltip("Maximum polyphony (1-8 voices)");

  // --- Anti-cacophony ---
  setupKnob(consonanceKnob, "Consonance", "consonance");
  consonanceKnob.slider.setTooltip(
      "Consonance filter: rejects dissonant intervals (0 = off, 1 = strict)");
  setupKnob(maxTrigsKnob, "MaxTrigs", "maxTriggersPerStep");
  maxTrigsKnob.slider.setTooltip(
      "Max triggers per step: caps simultaneous note-ons (1-8)");
  setupKnob(restProbKnob, "Rest%", "restProbability");
  restProbKnob.slider.setTooltip(
      "Rest probability: chance of full-step silence for rhythmic breathing");
  setupKnob(pitchGravityKnob, "Gravity", "pitchGravity");
  pitchGravityKnob.slider.setTooltip(
      "Pitch gravity: biases notes toward root, 5th, and 3rd chord tones");

  // --- CPU Meter ---
  cpuMeterLabel.setFont(nebulaLnF.getMonoFont(11.0f));
  cpuMeterLabel.setColour(juce::Label::textColourId, NebulaColours::text_dim);
  cpuMeterLabel.setJustificationType(juce::Justification::centredRight);
  cpuMeterLabel.setText("CPU: 0.0%", juce::dontSendNotification);
  addAndMakeVisible(cpuMeterLabel);

  // --- Transport controls ---
  playPauseBtn.setColour(juce::TextButton::buttonColourId,
                         NebulaColours::bg_surface);
  playPauseBtn.setColour(juce::TextButton::textColourOffId,
                         NebulaColours::accent1);
  playPauseBtn.onClick = [this]() {
    bool running = processor.engineRunning.load(std::memory_order_relaxed);
    processor.engineRunning.store(!running, std::memory_order_relaxed);
    playPauseBtn.setButtonText(running ? "Play" : "Pause");
  };
  playPauseBtn.setTooltip("Start or pause the cellular automaton");
  addAndMakeVisible(playPauseBtn);

  clearBtn.setColour(juce::TextButton::buttonColourId,
                     NebulaColours::bg_surface);
  clearBtn.setColour(juce::TextButton::textColourOffId,
                     NebulaColours::text_normal);
  clearBtn.onClick = [this]() {
    processor.clearRequested.store(true, std::memory_order_relaxed);
  };
  clearBtn.setTooltip("Clear the grid (kill all cells)");
  addAndMakeVisible(clearBtn);

  reseedBtn.setColour(juce::TextButton::buttonColourId,
                      NebulaColours::bg_surface);
  reseedBtn.setColour(juce::TextButton::textColourOffId,
                      NebulaColours::accent2);
  reseedBtn.onClick = [this]() {
    processor.reseedSymmetricRequested.store(true, std::memory_order_relaxed);
  };
  reseedBtn.setTooltip("Reseed the grid with a random symmetric pattern");
  addAndMakeVisible(reseedBtn);

  // --- Seed display ---
  seedLabel.setText("Seed:", juce::dontSendNotification);
  seedLabel.setFont(nebulaLnF.getMonoFont(10.0f));
  seedLabel.setColour(juce::Label::textColourId, NebulaColours::text_dim);
  seedLabel.setJustificationType(juce::Justification::centredRight);
  addAndMakeVisible(seedLabel);

  seedInput.setFont(nebulaLnF.getMonoFont(11.0f));
  seedInput.setColour(juce::TextEditor::backgroundColourId,
                      NebulaColours::bg_surface);
  seedInput.setColour(juce::TextEditor::textColourId,
                      NebulaColours::text_bright);
  seedInput.setColour(juce::TextEditor::outlineColourId,
                      NebulaColours::divider);
  seedInput.setText(
      juce::String::toHexString(static_cast<juce::int64>(processor.getSeed())),
      juce::dontSendNotification);
  seedInput.onReturnKey = [this]() {
    auto text = seedInput.getText().trim();
    auto val = text.getHexValue64();
    if (val != 0)
      processor.setSeed(static_cast<uint64_t>(val));
  };
  addAndMakeVisible(seedInput);

  // --- Symmetry ---
  setupCombo(symmetryCombo, "Symmetry", "symmetry");

  // --- Grid Size ---
  setupCombo(gridSizeCombo, "Grid", "gridSize");

  // --- Freeze toggle ---
  freezeBtn.setClickingTogglesState(true);
  freezeBtn.setColour(juce::TextButton::buttonColourId,
                      NebulaColours::bg_surface);
  freezeBtn.setColour(juce::TextButton::buttonOnColourId,
                      NebulaColours::accent1.withAlpha(0.7f));
  freezeBtn.setColour(juce::TextButton::textColourOffId,
                      NebulaColours::text_normal);
  freezeBtn.setColour(juce::TextButton::textColourOnId,
                      NebulaColours::text_bright);
  freezeBtn.setTooltip(
      "Freeze the grid: stop CA evolution while voices sustain");
  freezeAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          processor.getAPVTS(), "freeze", freezeBtn);
  addAndMakeVisible(freezeBtn);

  // --- New Seed button ---
  newSeedBtn.setColour(juce::TextButton::buttonColourId,
                       NebulaColours::bg_surface);
  newSeedBtn.setColour(juce::TextButton::textColourOffId,
                       NebulaColours::accent2);
  newSeedBtn.onClick = [this]() {
    // Generate a new random seed from system time
    auto now = juce::Time::currentTimeMillis();
    uint64_t newSeed = static_cast<uint64_t>(now);
    newSeed ^= newSeed << 13;
    newSeed ^= newSeed >> 7;
    newSeed ^= newSeed << 17;
    processor.setSeed(newSeed);
    processor.reseedSymmetricRequested.store(true, std::memory_order_relaxed);
    seedInput.setText(
        juce::String::toHexString(static_cast<juce::int64>(newSeed)),
        juce::dontSendNotification);
  };
  newSeedBtn.setTooltip("Generate a new random seed and reseed the grid");
  addAndMakeVisible(newSeedBtn);

  // --- FX popout button ---
  fxBtn.setColour(juce::TextButton::buttonColourId, NebulaColours::accent2_dim);
  fxBtn.setColour(juce::TextButton::textColourOffId,
                  NebulaColours::text_bright);
  fxBtn.onClick = [this]() {
    if (effectsWindow && effectsWindow->isVisible()) {
      effectsWindow->setVisible(false);
    } else {
      if (!effectsWindow)
        effectsWindow = std::make_unique<EffectsWindow>(processor, nebulaLnF);
      effectsWindow->setVisible(true);
      effectsWindow->toFront(true);
    }
  };
  fxBtn.setTooltip("Open the effects panel (chorus, delay, reverb, stereo)");
  addAndMakeVisible(fxBtn);

  // --- Factory pattern selector ---
  patternLabel.setText("Pattern", juce::dontSendNotification);
  patternLabel.setFont(nebulaLnF.getMonoFont(10.0f));
  patternLabel.setColour(juce::Label::textColourId, NebulaColours::text_dim);
  patternLabel.setJustificationType(juce::Justification::centredRight);
  addAndMakeVisible(patternLabel);

  patternCombo.setTextWhenNothingSelected("-- Pattern --");
  for (int i = 0; i < FactoryPatternLibrary::kPatternCount; ++i)
    patternCombo.addItem(FactoryPatternLibrary::getPattern(i).name, i + 1);
  patternCombo.onChange = [this]() {
    int idx = patternCombo.getSelectedId() - 1;
    if (idx >= 0)
      processor.loadPatternRequested.store(idx, std::memory_order_relaxed);
  };
  patternCombo.setTooltip("Load a classic Game of Life pattern into the grid");
  addAndMakeVisible(patternCombo);

  // --- MIDI Keyboard ---
  midiKeyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,
                         NebulaColours::accent1.withAlpha(0.5f));
  midiKeyboard.setColour(
      juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
      NebulaColours::accent2.withAlpha(0.3f));
  addAndMakeVisible(midiKeyboard);

  startTimerHz(10);
}

AlgoNebulaEditor::~AlgoNebulaEditor() {
  effectsWindow.reset(); // destroy before clearing LookAndFeel
  setLookAndFeel(nullptr);
}

//==============================================================================
void AlgoNebulaEditor::setupKnob(LabeledKnob &knob,
                                 const juce::String &labelText,
                                 const juce::String &paramID) {
  knob.slider.setSliderStyle(juce::Slider::Rotary);
  knob.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  knob.slider.setPopupDisplayEnabled(true, true, this);
  addAndMakeVisible(knob.slider);

  knob.label.setText(labelText, juce::dontSendNotification);
  knob.label.setJustificationType(juce::Justification::centred);
  knob.label.setColour(juce::Label::textColourId, NebulaColours::text_normal);
  addAndMakeVisible(knob.label);

  knob.attach =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          processor.getAPVTS(), paramID, knob.slider);
}

void AlgoNebulaEditor::setupCombo(LabeledCombo &combo,
                                  const juce::String &labelText,
                                  const juce::String &paramID) {
  auto *param = dynamic_cast<juce::AudioParameterChoice *>(
      processor.getAPVTS().getParameter(paramID));
  if (param != nullptr)
    combo.combo.addItemList(param->choices, 1);
  addAndMakeVisible(combo.combo);

  combo.label.setText(labelText, juce::dontSendNotification);
  combo.label.setJustificationType(juce::Justification::centred);
  combo.label.setColour(juce::Label::textColourId, NebulaColours::text_normal);
  addAndMakeVisible(combo.label);

  combo.attach =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          processor.getAPVTS(), paramID, combo.combo);
}

//==============================================================================
void AlgoNebulaEditor::paint(juce::Graphics &g) {
  // Dark gradient background
  juce::ColourGradient bgGradient(NebulaColours::bg_deepest, 0.0f, 0.0f,
                                  NebulaColours::bg_panel, 0.0f,
                                  static_cast<float>(getHeight()), false);
  g.setGradientFill(bgGradient);
  g.fillAll();

  // Title
  g.setColour(NebulaColours::text_bright);
  g.setFont(nebulaLnF.getInterFont(22.0f));
  g.drawText("Algo Nebula", 16, 8, 200, 30, juce::Justification::centredLeft);

  // Version
  g.setFont(nebulaLnF.getMonoFont(10.0f));
  g.setColour(NebulaColours::text_dim);
  g.drawText("v0.7.0", 16, 32, 80, 14, juce::Justification::centredLeft);

  // Section labels
  auto drawSectionLabel = [&](const juce::String &text, int x, int y) {
    g.setFont(nebulaLnF.getInterFont(11.0f));
    g.setColour(NebulaColours::accent1);
    g.drawText(text, x, y, 200, 14, juce::Justification::centredLeft);
  };

  const int margin = 16;
  const int headerH = 84;              // title (38) + selector (46)
  const int gridBottom = headerH + 38; // skip transport strip
  const int controlsAreaLeft = static_cast<int>(getWidth() * 0.52f);

  // Right-side section labels
  drawSectionLabel("ENVELOPE", controlsAreaLeft, gridBottom);
  drawSectionLabel("FILTER", controlsAreaLeft, gridBottom + 85);
  drawSectionLabel("MIX", controlsAreaLeft, gridBottom + 170);

  // Bottom sections
  int bottomY = static_cast<int>(getHeight() * 0.72f);
  drawSectionLabel("CLOCK", margin, bottomY);
  drawSectionLabel("TUNING", margin + 180, bottomY);
  drawSectionLabel("AMBIENT", margin + 360, bottomY);
  drawSectionLabel("HUMANIZE", margin + 560, bottomY);

  // Dividers
  g.setColour(NebulaColours::divider);
  g.drawLine(static_cast<float>(margin), headerH - 2.0f,
             static_cast<float>(getWidth() - margin), headerH - 2.0f, 1.0f);
}

//==============================================================================
void AlgoNebulaEditor::resized() {
  const int margin = 16;
  const int titleH = 38;
  const int selectorH = 46; // 14 label + 4 gap + 24 combo + 4 pad
  const int labelH = 14;
  const int knobSize = 52;
  const int comboH = 24;

  auto area = getLocalBounds();

  // --- Header: title row + selector row ---
  cpuMeterLabel.setBounds(getWidth() - 110, 12, 94, 14);
  presetLabel.setBounds(220, 10, 50, 20);
  presetCombo.setBounds(275, 8, 200, 24);

  // --- Top selector row (below title) ---
  auto titleRow = area.removeFromTop(titleH);
  auto selectorRow = area.removeFromTop(selectorH).reduced(margin, 0);
  selectorRow.removeFromTop(4);
  const int comboW = (selectorRow.getWidth() - 30) / 4;

  auto layoutOneCombo = [&](LabeledCombo &cb, juce::Rectangle<int> bounds) {
    cb.label.setBounds(bounds.removeFromTop(labelH));
    bounds.removeFromTop(2);
    cb.combo.setBounds(bounds.withHeight(comboH));
  };

  auto a1 = selectorRow.removeFromLeft(comboW);
  selectorRow.removeFromLeft(10);
  auto a2 = selectorRow.removeFromLeft(comboW);
  selectorRow.removeFromLeft(10);
  auto a3 = selectorRow.removeFromLeft(comboW);
  selectorRow.removeFromLeft(10);
  auto a4 = selectorRow;

  layoutOneCombo(algorithmCombo, a1);
  layoutOneCombo(scaleCombo, a2);
  layoutOneCombo(keyCombo, a3);
  layoutOneCombo(waveshapeCombo, a4);

  area.removeFromTop(4);

  // --- Transport strip (between header and grid) ---
  {
    auto transportRow = area.removeFromTop(28).reduced(margin, 2);
    int btnW = 60;
    playPauseBtn.setBounds(transportRow.removeFromLeft(btnW));
    transportRow.removeFromLeft(4);
    clearBtn.setBounds(transportRow.removeFromLeft(btnW));
    transportRow.removeFromLeft(4);
    reseedBtn.setBounds(transportRow.removeFromLeft(btnW));
    transportRow.removeFromLeft(4);
    freezeBtn.setBounds(transportRow.removeFromLeft(btnW));
    transportRow.removeFromLeft(4);
    newSeedBtn.setBounds(transportRow.removeFromLeft(70));
    transportRow.removeFromLeft(8);

    // FX popout button
    fxBtn.setBounds(transportRow.removeFromLeft(36).reduced(0, 1));

    // Pattern combo
    patternLabel.setBounds(transportRow.removeFromLeft(50));
    patternCombo.setBounds(transportRow.removeFromLeft(100).reduced(0, 1));
    transportRow.removeFromLeft(8);

    // Symmetry combo (no label in strip, compact)
    symmetryCombo.label.setBounds(transportRow.removeFromLeft(60));
    symmetryCombo.combo.setBounds(
        transportRow.removeFromLeft(110).reduced(0, 1));
    transportRow.removeFromLeft(8);

    // Grid Size combo
    gridSizeCombo.label.setBounds(transportRow.removeFromLeft(30));
    gridSizeCombo.combo.setBounds(
        transportRow.removeFromLeft(90).reduced(0, 1));
    transportRow.removeFromLeft(12);

    // Seed display
    seedLabel.setBounds(transportRow.removeFromLeft(36));
    transportRow.removeFromLeft(4);
    seedInput.setBounds(transportRow.removeFromLeft(130).reduced(0, 1));
  }
  area.removeFromTop(2);

  // --- Middle area: grid (left) + synth controls (right) ---
  const int controlsW = static_cast<int>(getWidth() * 0.48f);
  auto middleArea = area.removeFromTop(static_cast<int>(getHeight() * 0.55f));

  // Grid takes left side
  auto gridArea = middleArea.removeFromLeft(getWidth() - controlsW - margin)
                      .reduced(margin, 4);
  gridComponent.setBounds(gridArea);

  // Right side controls
  auto ctrlArea = middleArea.reduced(4, 0);

  // Helper for laying out a row of knobs
  auto layoutKnobs = [&](juce::Rectangle<int> row,
                         std::initializer_list<LabeledKnob *> knobs) {
    int n = static_cast<int>(knobs.size());
    int kw = row.getWidth() / n;
    for (auto *k : knobs) {
      auto cell = row.removeFromLeft(kw);
      k->label.setBounds(cell.removeFromBottom(labelH));
      int sz = std::min(knobSize, cell.getWidth());
      k->slider.setBounds(
          cell.withSizeKeepingCentre(sz, std::min(sz, cell.getHeight())));
    }
  };

  // Envelope section (5 knobs)
  auto envArea = ctrlArea.removeFromTop(14); // label space
  envArea = ctrlArea.removeFromTop(knobSize + labelH + 4);
  layoutKnobs(envArea,
              {&attackKnob, &holdKnob, &decayKnob, &sustainKnob, &releaseKnob});

  ctrlArea.removeFromTop(8);

  // Filter section (2 knobs + combo)
  ctrlArea.removeFromTop(14); // label space
  auto filterRow = ctrlArea.removeFromTop(knobSize + labelH + 4);
  int filterItemW = filterRow.getWidth() / 3;
  {
    auto cell = filterRow.removeFromLeft(filterItemW);
    filterCutoffKnob.label.setBounds(cell.removeFromBottom(labelH));
    int sz = std::min(knobSize, cell.getWidth());
    filterCutoffKnob.slider.setBounds(
        cell.withSizeKeepingCentre(sz, std::min(sz, cell.getHeight())));
  }
  {
    auto cell = filterRow.removeFromLeft(filterItemW);
    filterResKnob.label.setBounds(cell.removeFromBottom(labelH));
    int sz = std::min(knobSize, cell.getWidth());
    filterResKnob.slider.setBounds(
        cell.withSizeKeepingCentre(sz, std::min(sz, cell.getHeight())));
  }
  {
    auto cell = filterRow;
    filterModeCombo.label.setBounds(cell.removeFromTop(labelH));
    cell.removeFromTop(4);
    filterModeCombo.combo.setBounds(cell.withHeight(comboH).reduced(4, 0));
  }

  ctrlArea.removeFromTop(8);

  // Mix section (3 knobs + combo)
  ctrlArea.removeFromTop(14); // label space
  auto mixRow = ctrlArea.removeFromTop(knobSize + labelH + 4);
  int mixItemW = mixRow.getWidth() / 4;
  {
    auto cell = mixRow.removeFromLeft(mixItemW);
    noiseLevelKnob.label.setBounds(cell.removeFromBottom(labelH));
    int sz = std::min(knobSize, cell.getWidth());
    noiseLevelKnob.slider.setBounds(
        cell.withSizeKeepingCentre(sz, std::min(sz, cell.getHeight())));
  }
  {
    auto cell = mixRow.removeFromLeft(mixItemW);
    waveshapeSpreadKnob.label.setBounds(cell.removeFromBottom(labelH));
    int sz = std::min(knobSize, cell.getWidth());
    waveshapeSpreadKnob.slider.setBounds(
        cell.withSizeKeepingCentre(sz, std::min(sz, cell.getHeight())));
  }
  {
    auto cell = mixRow.removeFromLeft(mixItemW);
    subLevelKnob.label.setBounds(cell.removeFromBottom(labelH));
    int sz = std::min(knobSize, cell.getWidth());
    subLevelKnob.slider.setBounds(
        cell.withSizeKeepingCentre(sz, std::min(sz, cell.getHeight())));
  }
  {
    auto cell = mixRow;
    subOctaveCombo.label.setBounds(cell.removeFromTop(labelH));
    cell.removeFromTop(4);
    subOctaveCombo.combo.setBounds(cell.withHeight(comboH).reduced(4, 0));
  }

  // --- Bottom area: Clock | Tuning | Ambient | Humanize | Global ---
  area.removeFromTop(4);
  auto bottomArea = area.reduced(margin, 0);

  // Clock section
  int sectionW = bottomArea.getWidth() / 6;
  ctrlArea.removeFromTop(14); // for label
  auto clockArea = bottomArea.removeFromLeft(sectionW);
  clockArea.removeFromTop(14); // label gap
  auto clockKnobRow = clockArea.removeFromTop(knobSize + labelH + 4);
  int clockItemW = clockKnobRow.getWidth() / 3;
  {
    auto cell = clockKnobRow.removeFromLeft(clockItemW);
    bpmKnob.label.setBounds(cell.removeFromBottom(labelH));
    int sz = std::min(knobSize, cell.getWidth());
    bpmKnob.slider.setBounds(
        cell.withSizeKeepingCentre(sz, std::min(sz, cell.getHeight())));
  }
  {
    auto cell = clockKnobRow.removeFromLeft(clockItemW);
    clockDivCombo.label.setBounds(cell.removeFromTop(labelH));
    cell.removeFromTop(2);
    clockDivCombo.combo.setBounds(cell.withHeight(comboH).reduced(2, 0));
  }
  {
    auto cell = clockKnobRow;
    swingKnob.label.setBounds(cell.removeFromBottom(labelH));
    int sz = std::min(knobSize, cell.getWidth());
    swingKnob.slider.setBounds(
        cell.withSizeKeepingCentre(sz, std::min(sz, cell.getHeight())));
  }

  // Tuning section
  auto tuneArea = bottomArea.removeFromLeft(sectionW);
  tuneArea.removeFromTop(14);
  auto tuneRow = tuneArea.removeFromTop(knobSize + labelH + 4);
  {
    auto cell = tuneRow.removeFromLeft(tuneRow.getWidth() / 2);
    tuningCombo.label.setBounds(cell.removeFromTop(labelH));
    cell.removeFromTop(2);
    tuningCombo.combo.setBounds(cell.withHeight(comboH).reduced(2, 0));
  }
  {
    auto cell = tuneRow;
    refPitchKnob.label.setBounds(cell.removeFromBottom(labelH));
    int sz = std::min(knobSize, cell.getWidth());
    refPitchKnob.slider.setBounds(
        cell.withSizeKeepingCentre(sz, std::min(sz, cell.getHeight())));
  }

  // Ambient section (3 knobs)
  auto ambArea = bottomArea.removeFromLeft(sectionW);
  ambArea.removeFromTop(14);
  auto ambRow = ambArea.removeFromTop(knobSize + labelH + 4);
  layoutKnobs(ambRow, {&droneSustainKnob, &noteProbKnob, &gateTimeKnob});

  // Anti-cacophony section (4 knobs)
  auto cacArea = bottomArea.removeFromLeft(sectionW);
  cacArea.removeFromTop(14);
  auto cacRow = cacArea.removeFromTop(knobSize + labelH + 4);
  layoutKnobs(cacRow, {&consonanceKnob, &maxTrigsKnob, &restProbKnob,
                       &pitchGravityKnob});

  // Humanize section (4 knobs)
  auto humArea = bottomArea.removeFromLeft(sectionW);
  humArea.removeFromTop(14);
  auto humRow = humArea.removeFromTop(knobSize + labelH + 4);
  layoutKnobs(humRow, {&strumSpreadKnob, &melodicInertiaKnob, &roundRobinKnob,
                       &velHumanizeKnob});

  // Global section (volume + voices)
  auto globalArea = bottomArea;
  globalArea.removeFromTop(14);
  auto globalRow = globalArea.removeFromTop(knobSize + labelH + 4);
  layoutKnobs(globalRow, {&masterVolumeKnob, &voiceCountKnob});

  // --- MIDI Keyboard at very bottom ---
  auto keyboardArea = getLocalBounds().removeFromBottom(64).reduced(margin, 4);
  midiKeyboard.setBounds(keyboardArea);
}

//==============================================================================
void AlgoNebulaEditor::timerCallback() {
  float cpu = processor.getCpuLoadPercent();
  cpuMeterLabel.setText(juce::String("CPU: ") + juce::String(cpu, 1) + "%",
                        juce::dontSendNotification);

  if (cpu > 80.0f)
    cpuMeterLabel.setColour(juce::Label::textColourId, NebulaColours::danger);
  else if (cpu > 50.0f)
    cpuMeterLabel.setColour(juce::Label::textColourId, NebulaColours::warning);
  else
    cpuMeterLabel.setColour(juce::Label::textColourId, NebulaColours::text_dim);

  // Update seed display (only when user is not typing)
  if (!seedInput.hasKeyboardFocus(false)) {
    auto seedHex = juce::String::toHexString(
        static_cast<juce::int64>(processor.getSeed()));
    if (seedInput.getText() != seedHex)
      seedInput.setText(seedHex, juce::dontSendNotification);
  }

  // Update play/pause button text to match state
  bool running = processor.engineRunning.load(std::memory_order_relaxed);
  playPauseBtn.setButtonText(running ? "Pause" : "Play");
}
