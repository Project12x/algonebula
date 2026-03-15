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
  presetCombo.setTooltip(
      "Factory preset: instantly load a curated combination of algorithm, scale, "
      "envelope, effects, and musicality settings. Great starting points for " 
      "exploration -- tweak any parameter after loading.");
  addAndMakeVisible(presetCombo);

  // --- Top selectors ---
  setupCombo(algorithmCombo, "Algorithm", "algorithm");
  algorithmCombo.combo.setTooltip(
      "Cellular automaton engine. Each algorithm produces different visual and "
      "musical patterns: Game of Life (rhythmic pulses), Brian's Brain (rapid "
      "bursts), Cyclic CA (rotating spirals), Reaction-Diffusion (slow evolving "
      "textures), Lenia (organic morphing), Brownian Field (scattered ambient), "
      "Particle Swarm (directional movement).");
  setupCombo(scaleCombo, "Scale", "scale");
  scaleCombo.combo.setTooltip(
      "Musical scale: constrains all generated notes to this scale. Chromatic = "
      "all 12 notes, Major/Minor = Western harmony, Pentatonic = safe/consonant, "
      "Whole Tone = dreamy/ambiguous, Blues = gritty character. Every note the "
      "engine produces is quantized to this scale.");
  setupCombo(keyCombo, "Key", "key");
  keyCombo.combo.setTooltip(
      "Root key: the tonal center of the scale. Change this to transpose all "
      "output up or down. C is standard, but try different keys to match other "
      "instruments or find a register you prefer.");
  setupCombo(waveshapeCombo, "Wave", "waveshape");
  waveshapeCombo.combo.setTooltip(
      "Oscillator waveform: Sine (pure/warm), Triangle (soft/mellow), Saw "
      "(bright/rich harmonics), Pulse (hollow/reedy), Sine+Oct (organ-like), "
      "Fifth Stack (power chord), Pad (detuned unison shimmer), Bell (metallic "
      "FM tones). Use Waveshape Spread to mix multiple shapes across voices.");

  // --- Clock ---
  setupKnob(bpmKnob, "BPM", "bpm");
  bpmKnob.slider.setTooltip(
      "Tempo (40-300 BPM): controls how fast the cellular automaton steps. "
      "Lower = slower evolution, more ambient. Higher = faster patterns, more "
      "rhythmic. Interacts with Clock Division to set actual step rate.");
  setupCombo(clockDivCombo, "Clock", "clockDiv");
  clockDivCombo.combo.setTooltip(
      "Clock division: subdivides the BPM tempo. 1/1 = one step per beat, "
      "1/4 = four steps per beat, 1/16 = very rapid stepping. Lower divisions "
      "give the grid more time to evolve between note triggers.");
  setupKnob(swingKnob, "Swing", "swing");
  swingKnob.slider.setTooltip(
      "Swing (0-100%): delays every other grid step for a shuffle feel. "
      "0% = perfectly straight timing, 67% = classic triplet swing, 100% = "
      "extreme dotted-note feel. Adds groove and human feel to the output.");

  // --- Envelope ---
  setupKnob(attackKnob, "Atk", "attack");
  attackKnob.slider.setTooltip(
      "Attack (0-2s): how quickly notes reach full volume. Short = percussive "
      "plucks, long = slow swells. Try long attack + long release for pad-like "
      "ambient textures.");
  setupKnob(holdKnob, "Hold", "hold");
  holdKnob.slider.setTooltip(
      "Hold (0-2s): how long the note stays at peak volume before decay begins. "
      "0 = immediate decay after attack, higher = more sustained presence. "
      "Useful for bell-like or organ sounds.");
  setupKnob(decayKnob, "Dcy", "decay");
  decayKnob.slider.setTooltip(
      "Decay (0-5s): how quickly the note fades from peak to sustain level. "
      "Short = snappy, rhythmic. Long = gradual fade, cinematic. Works with "
      "Sustain to shape the body of each note.");
  setupKnob(sustainKnob, "Sus", "sustain");
  sustainKnob.slider.setTooltip(
      "Sustain (0-100%): volume level the note holds at after decay. 0% = note "
      "dies after decay (percussive), 100% = no decay at all (organ-like). "
      "Most ambient sounds work well around 30-60%.");
  setupKnob(releaseKnob, "Rel", "release");
  releaseKnob.slider.setTooltip(
      "Release (0-10s): how long the note rings after the cell dies. Short = "
      "notes cut off cleanly, long = notes fade out gradually creating overlapping "
      "textures. High release + reverb = lush ambient washes.");

  // --- Filter ---
  setupKnob(filterCutoffKnob, "Cutoff", "filterCutoff");
  filterCutoffKnob.slider.setTooltip(
      "Filter cutoff (20 Hz - 20 kHz): frequency where the filter begins to take "
      "effect. Low values = dark/muffled, high = bright/open. In LP mode, "
      "everything above cutoff is reduced. The grid density also modulates "
      "this in real-time.");
  setupKnob(filterResKnob, "Reso", "filterRes");
  filterResKnob.slider.setTooltip(
      "Filter resonance (0-100%): boosts frequencies near the cutoff, creating "
      "a vocal-like quality. Low = gentle shaping, high = sharp resonant peak. "
      "High resonance + sweeping cutoff = classic synth filter sound.");
  setupCombo(filterModeCombo, "Filter", "filterMode");
  filterModeCombo.combo.setTooltip(
      "Filter type: LP (low pass) = removes highs for warmth, HP (high pass) = "
      "removes lows for thinness/clarity, BP (band pass) = isolates a frequency "
      "band, Notch = removes one band. LP is the most commonly used for ambient.");

  // --- Mix ---
  setupKnob(noiseLevelKnob, "Noise", "noiseLevel");
  noiseLevelKnob.slider.setTooltip(
      "Noise level (0-100%): mixes white noise into each voice. Adds texture, "
      "breathiness, or percussive attack to the sound. Low amounts add air, "
      "high amounts create noise-based textures or wind effects.");
  setupKnob(waveshapeSpreadKnob, "WSpread", "waveshapeSpread");
  waveshapeSpreadKnob.slider.setTooltip(
      "Waveshape spread (0-100%): distributes different waveforms across voices. "
      "0% = all voices use the selected waveshape, 100% = each voice cycles "
      "through different shapes (sine, saw, pulse, etc). Creates richer, more "
      "complex textures from the same notes.");
  setupKnob(subLevelKnob, "Sub", "subLevel");
  subLevelKnob.slider.setTooltip(
      "Sub-oscillator level (0-100%): adds a pure sine wave 1-2 octaves below "
      "the lowest active voice. Adds weight and bass foundation without muddiness. "
      "Essential for filling out thin patches or creating deep drones.");
  setupCombo(subOctaveCombo, "Sub Oct", "subOctave");
  subOctaveCombo.combo.setTooltip(
      "Sub-oscillator octave: -1 oct = one octave below (warm thickening), "
      "-2 oct = two octaves below (deep sub-bass rumble). Choose based on how "
      "much low-end weight you want.");

  // --- Tuning ---
  setupCombo(tuningCombo, "Tuning", "tuning");
  tuningCombo.combo.setTooltip(
      "Tuning system: 12-TET = standard equal temperament (familiar), Just "
      "Intonation = pure ratio intervals (beatless chords, slightly different "
      "feel), Pythagorean = pure 5ths (bright, medieval character). Most people "
      "prefer 12-TET. Try Just for ambient 5th-based harmony.");
  setupKnob(refPitchKnob, "A4 Hz", "refPitch");
  refPitchKnob.slider.setTooltip(
      "A4 reference pitch (420-460 Hz): the tuning standard. 440 Hz = concert "
      "pitch, 432 Hz = popular alternative (slightly warmer), 442-444 Hz = "
      "orchestral bright tuning. Shifts all notes up or down together.");

  // --- Ambient ---
  setupKnob(droneSustainKnob, "Drone", "droneSustain");
  droneSustainKnob.slider.setTooltip(
      "Drone sustain (0-100%): extends voice lifetime beyond cell death. 0% = "
      "notes stop immediately when cells die, 100% = voices ring indefinitely, "
      "creating evolving drone layers. Higher values build up dense ambient "
      "washes as notes overlap.");
  setupKnob(noteProbKnob, "Prob", "noteProbability");
  noteProbKnob.slider.setTooltip(
      "Note probability (0-100%): chance that each active cell triggers a note. "
      "100% = every cell plays, 50% = half are skipped randomly, creating "
      "sparser textures. Lower values = more spacious, meditative. Works with "
      "Rest Probability for rhythmic control.");
  setupKnob(gateTimeKnob, "Gate", "gateTime");
  gateTimeKnob.slider.setTooltip(
      "Gate time (0-100%): note duration relative to step length. 100% = legato "
      "(notes fill entire step, overlapping), 50% = staccato (short detached "
      "notes), 10% = ultra-short percussive hits. Lower values create more "
      "rhythmic, bouncy patterns.");

  // --- Humanize ---
  setupKnob(strumSpreadKnob, "Strum", "strumSpread");
  strumSpreadKnob.slider.setTooltip(
      "Strum spread (0-50 ms): delays note onsets based on grid column position, "
      "like strumming a guitar. 0 = all notes hit simultaneously, higher = "
      "cascading left-to-right spread. Adds a natural, organic quality to "
      "dense chords.");
  setupKnob(melodicInertiaKnob, "Inertia", "melodicInertia");
  melodicInertiaKnob.slider.setTooltip(
      "Melodic inertia (0-100%): tendency to repeat or stay near the last pitch. "
      "0% = every note is independent (chaotic), 50% = mix of repetition and "
      "movement, 100% = notes cluster around the same pitch (droning). "
      "Creates melodic continuity from otherwise random patterns.");
  setupKnob(roundRobinKnob, "RndRbn", "roundRobin");
  roundRobinKnob.slider.setTooltip(
      "Round-robin (0-100%): cycles voice allocation to avoid the same voice "
      "retriggering repeatedly. Higher values = more variation in which voice "
      "plays each note, preventing robotic repetition and adding natural feel.");
  setupKnob(velHumanizeKnob, "VelHum", "velocityHumanize");
  velHumanizeKnob.slider.setTooltip(
      "Velocity humanize (0-100%): adds random variation to note volume. "
      "0% = all notes at equal volume (mechanical), higher = some notes louder, "
      "some softer (human feel). Creates dynamic contrast and prevents the "
      "flat, lifeless sound of uniform velocity.");

  // --- Global ---
  setupKnob(masterVolumeKnob, "Volume", "masterVolume");
  masterVolumeKnob.slider.setTooltip(
      "Master volume (0-200%): final output level. 100% = unity gain, above "
      "100% = boost (watch for clipping). The built-in safety limiter prevents "
      "dangerous levels, but lower is generally safer for mixing.");
  setupKnob(voiceCountKnob, "Voices", "voiceCount");
  voiceCountKnob.slider.setTooltip(
      "Max voices (1-8): maximum simultaneous notes. 1 = monophonic (one note "
      "at a time), 4 = moderate density, 8 = full polyphony. More voices = "
      "richer texture but higher CPU. Dense algorithms may sound better with "
      "fewer voices.");

  // --- Anti-cacophony ---
  setupKnob(consonanceKnob, "Consonance", "consonance");
  consonanceKnob.slider.setTooltip(
      "Consonance filter (0-100%): prevents clashing intervals between "
      "simultaneous notes. 0% = anything goes (dissonant/experimental), 50% = "
      "mild filtering (some tension allowed), 100% = strict consonance "
      "(only 3rds, 5ths, octaves). Higher = safer harmony, lower = edgier.");
  setupKnob(maxTrigsKnob, "MaxTrigs", "maxTriggersPerStep");
  maxTrigsKnob.slider.setTooltip(
      "Max triggers per step (1-8): limits how many new notes can start in one "
      "grid step. Prevents overwhelming bursts from dense algorithms. 1-2 = "
      "sparse, melodic, 4-6 = moderate chordal, 8 = full density. Lower values "
      "tame chaotic algorithms into musical results.");
  setupKnob(restProbKnob, "Rest%", "restProbability");
  restProbKnob.slider.setTooltip(
      "Rest probability (0-100%): chance that an entire grid step is silent. "
      "Creates rhythmic breathing and space in the output. 0% = constant "
      "notes, 30% = occasional pauses, 80% = mostly silence with sparse notes. "
      "Essential for preventing wall-of-sound fatigue.");
  setupKnob(pitchGravityKnob, "Gravity", "pitchGravity");
  pitchGravityKnob.slider.setTooltip(
      "Pitch gravity (0-100%): pulls notes toward chord tones (root, 3rd, 5th) "
      "of the selected scale. 0% = notes land on any scale degree equally, "
      "100% = strong bias toward root and 5th (more tonal/grounded). Higher "
      "values create a stronger sense of key center.");

  // --- CPU Meter ---
  cpuMeterLabel.setFont(nebulaLnF.getMonoFont(11.0f));
  cpuMeterLabel.setColour(juce::Label::textColourId, NebulaColours::text_dim);
  cpuMeterLabel.setJustificationType(juce::Justification::centredRight);
  cpuMeterLabel.setText("CPU: 0.0%", juce::dontSendNotification);
  addAndMakeVisible(cpuMeterLabel);

  gpuMeterLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
  gpuMeterLabel.setColour(juce::Label::textColourId, NebulaColours::text_dim);
  gpuMeterLabel.setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(gpuMeterLabel);

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
  playPauseBtn.setTooltip(
      "Play/Pause: starts or stops the grid evolution. When paused, the grid "
      "freezes and no new notes trigger, but existing voices finish naturally.");
  addAndMakeVisible(playPauseBtn);

  clearBtn.setColour(juce::TextButton::buttonColourId,
                     NebulaColours::bg_surface);
  clearBtn.setColour(juce::TextButton::textColourOffId,
                     NebulaColours::text_normal);
  clearBtn.onClick = [this]() {
    processor.clearRequested.store(true, std::memory_order_relaxed);
  };
  clearBtn.setTooltip(
      "Clear: kills all cells in the grid. Active voices will fade out based on "
      "release time. The grid remains empty until you reseed or draw new cells.");
  addAndMakeVisible(clearBtn);

  reseedBtn.setColour(juce::TextButton::buttonColourId,
                      NebulaColours::bg_surface);
  reseedBtn.setColour(juce::TextButton::textColourOffId,
                      NebulaColours::accent2);
  reseedBtn.onClick = [this]() {
    processor.reseedSymmetricRequested.store(true, std::memory_order_relaxed);
  };
  reseedBtn.setTooltip(
      "Reseed: fills the grid with a new random symmetric pattern based on the "
      "current seed and symmetry mode. Good for generating fresh starting "
      "points when the current pattern stagnates.");
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
      "Freeze: halts the cellular automaton evolution while active voices "
      "continue to sustain. The grid pattern is locked in place -- useful "
      "for holding a particular harmonic moment or creating a static drone.");
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
  newSeedBtn.setTooltip(
      "New Seed: generates a fresh random seed number and immediately reseeds "
      "the grid. Each seed produces a unique but reproducible pattern -- write "
      "down seeds you like to recreate them later.");
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
  fxBtn.setTooltip(
      "Effects panel: opens a separate window for chorus, delay, reverb, and "
      "stereo width controls. Each effect has its own on/off toggle, mix level, "
      "and parameters. Effects can dramatically change the character of the "
      "output.");
  addAndMakeVisible(fxBtn);

  // --- GPU Acceleration toggle ---
  gpuAccelBtn.setClickingTogglesState(true);
  gpuAccelBtn.setColour(juce::TextButton::buttonColourId,
                        juce::Colour(0xFF1B4D4D));
  gpuAccelBtn.setColour(juce::TextButton::buttonOnColourId,
                        juce::Colour(0xFF00CC88));
  gpuAccelBtn.setColour(juce::TextButton::textColourOffId,
                        NebulaColours::text_normal);
  gpuAccelBtn.setColour(juce::TextButton::textColourOnId,
                        NebulaColours::text_bright);
  gpuAccelBtn.setTooltip(
      "GPU Acceleration: offloads cellular automaton simulation to the GPU via "
      "WebGPU compute shaders. Enables higher-fidelity simulations for "
      "Lenia, Reaction-Diffusion, and Particle Swarm. Falls back to CPU if "
      "GPU is unavailable. May improve performance on larger grid sizes.");
  gpuAccelAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          processor.getAPVTS(), "gpuAccel", gpuAccelBtn);
  addAndMakeVisible(gpuAccelBtn);

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
  patternCombo.setTooltip(
      "Factory patterns: load classic Game of Life structures. Glider = small "
      "moving pattern, LWSS = larger spaceship, R-Pentomino = chaotic growth "
      "from 5 cells, Pulsar = symmetric oscillator, Gosper Gun = continuously "
      "produces gliders. Works best with the Game of Life algorithm.");
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
  gpuMeterLabel.setBounds(getWidth() - 200, 12, 90, 14);
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
    transportRow.removeFromLeft(4);

    // GPU toggle
    gpuAccelBtn.setBounds(transportRow.removeFromLeft(40).reduced(0, 1));

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

  // Update GPU button text to reflect actual activation state
  bool gpuOn = processor.isGpuActive();
  gpuAccelBtn.setButtonText(gpuOn ? "GPU ON" : "GPU");

  // Update GPU meter
  if (gpuOn) {
    float gpuMs = processor.getGpuStepMs();
    float gpuPct = (gpuMs / 16.0f) * 100.0f; // % of 16ms frame
    gpuMeterLabel.setText(juce::String("GPU: ") + juce::String(gpuMs, 1) + "ms", juce::dontSendNotification);
    if (gpuPct > 80.0f) gpuMeterLabel.setColour(juce::Label::textColourId, NebulaColours::danger);
    else if (gpuPct > 50.0f) gpuMeterLabel.setColour(juce::Label::textColourId, NebulaColours::warning);
    else gpuMeterLabel.setColour(juce::Label::textColourId, NebulaColours::alive);
  } else {
    gpuMeterLabel.setText("", juce::dontSendNotification);
  }
}