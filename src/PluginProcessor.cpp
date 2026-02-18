#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AlgoNebulaProcessor::AlgoNebulaProcessor()
    : AudioProcessor(BusesProperties().withOutput(
          "Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "AlgoNebulaState", createParameterLayout()),
      engine(createEngine(0)) {}

std::unique_ptr<CellularEngine>
AlgoNebulaProcessor::createEngine(int algoIdx, int rows, int cols) {
  switch (algoIdx) {
  case 0:
    return std::make_unique<GameOfLife>(rows, cols,
                                        GameOfLife::RulePreset::Classic);
  case 1:
    return std::make_unique<GameOfLife>(rows, cols,
                                        GameOfLife::RulePreset::HighLife);
  case 2:
    return std::make_unique<BriansBrain>(rows, cols);
  case 3:
    return std::make_unique<CyclicCA>(rows, cols);
  case 4:
    return std::make_unique<ReactionDiffusion>(rows, cols);
  case 5:
    return std::make_unique<ParticleSwarm>(rows, cols);
  case 6:
    return std::make_unique<LeniaEngine>(rows, cols);
  case 7:
    return std::make_unique<BrownianField>(rows, cols);
  default:
    return std::make_unique<GameOfLife>(rows, cols,
                                        GameOfLife::RulePreset::Classic);
  }
}

AlgoNebulaProcessor::~AlgoNebulaProcessor() = default;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
AlgoNebulaProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  // --- Master ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("masterVolume", 1), "Master Volume",
      juce::NormalisableRange<float>(0.0f, 2.0f, 0.001f), 0.5f));

  // --- Algorithm ---
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("algorithm", 1), "Algorithm",
      juce::StringArray{"Game of Life", "Wolfram 1D", "Brian's Brain",
                        "Cyclic CA", "Reaction-Diffusion", "Particle Swarm",
                        "Lenia", "Brownian Field"},
      0));

  // --- Clock ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("bpm", 1), "BPM",
      juce::NormalisableRange<float>(40.0f, 300.0f, 0.1f), 120.0f));

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("clockDiv", 1), "Clock Division",
      juce::StringArray{"1/1", "1/2", "1/4", "1/8", "1/16", "1/32"}, 2));

  // --- Scale ---
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("scale", 1), "Scale",
      juce::StringArray{"Chromatic", "Major", "Minor", "Dorian", "Phrygian",
                        "Lydian", "Mixolydian", "Aeolian", "Locrian",
                        "Pent. Major", "Pent. Minor", "Blues", "Whole Tone",
                        "Harmonic Minor", "Melodic Minor"},
      1)); // Major

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("key", 1), "Key",
      juce::StringArray{"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A",
                        "A#", "B"},
      0));

  // --- Voices ---
  layout.add(std::make_unique<juce::AudioParameterInt>(
      juce::ParameterID("voiceCount", 1), "Voice Count", 1, 64, 3));

  // --- Waveshape ---
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("waveshape", 1), "Waveshape",
      juce::StringArray{"Sine", "Triangle", "Saw", "Pulse", "Sine+Oct",
                        "Fifth Stack", "Pad", "Bell"},
      0));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("waveshapeSpread", 1), "Waveshape Spread",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  // --- Ambient ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("droneSustain", 1), "Drone Sustain",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("noteProbability", 1), "Note Probability",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("gateTime", 1), "Gate Time",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

  // --- Humanization ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("swing", 1), "Swing",
      juce::NormalisableRange<float>(50.0f, 75.0f, 0.1f), 50.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("strumSpread", 1), "Strum Spread",
      juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("melodicInertia", 1), "Melodic Inertia",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("roundRobin", 1), "Round Robin",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.2f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("velocityHumanize", 1), "Velocity Humanize",
      juce::NormalisableRange<float>(0.0f, 0.5f, 0.01f), 0.1f));

  // --- Envelope ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("attack", 1), "Attack",
      juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.8f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("hold", 1), "Hold",
      juce::NormalisableRange<float>(0.0f, 2.0f, 0.001f, 0.5f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("decay", 1), "Decay",
      juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.5f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("sustain", 1), "Sustain",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("release", 1), "Release",
      juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 3.0f));

  // --- Filter ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("filterCutoff", 1), "Filter Cutoff",
      juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 8000.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("filterRes", 1), "Filter Resonance",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("filterMode", 1), "Filter Mode",
      juce::StringArray{"Low Pass", "High Pass", "Band Pass", "Notch"}, 0));

  // --- Noise + Sub ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("noiseLevel", 1), "Noise Level",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("subLevel", 1), "Sub Level",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("subOctave", 1), "Sub Octave",
      juce::StringArray{"-1 Oct", "-2 Oct"}, 0));

  // --- Tuning ---
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("tuning", 1), "Tuning",
      juce::StringArray{"12-TET", "Just Intonation", "Pythagorean"}, 0));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("refPitch", 1), "Reference Pitch",
      juce::NormalisableRange<float>(420.0f, 460.0f, 0.1f), 440.0f));

  // --- Symmetry ---
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("symmetry", 1), "Symmetry",
      juce::StringArray{"None", "4-Fold Mirror"}, 1)); // default to symmetric

  // --- Grid Resolution ---
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("gridSize", 1), "Grid Size",
      juce::StringArray{"Small (8x12)", "Medium (12x16)", "Large (16x24)",
                        "XL (24x32)", "XXL (32x48)", "Epic (48x64)",
                        "Massive (64x96)", "Huge (128x128)",
                        "Experimental (256x256)"},
      1)); // default to Medium

  // --- Freeze ---
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("freeze", 1), "Freeze", juce::StringArray{"Off", "On"},
      0));

  // --- Anti-cacophony ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("consonance", 1), "Consonance",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

  layout.add(std::make_unique<juce::AudioParameterInt>(
      juce::ParameterID("maxTriggersPerStep", 1), "Max Triggers/Step", 1, 8,
      3));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("restProbability", 1), "Rest Probability",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.2f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("pitchGravity", 1), "Pitch Gravity",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

  // --- Stereo Width ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("stereoWidth", 1), "Stereo Width",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

  // --- Chorus ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("chorusRate", 1), "Chorus Rate",
      juce::NormalisableRange<float>(0.1f, 5.0f, 0.01f), 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("chorusDepth", 1), "Chorus Depth",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.4f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("chorusMix", 1), "Chorus Mix",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  // --- Delay ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("delayTime", 1), "Delay Time",
      juce::NormalisableRange<float>(0.01f, 2.0f, 0.01f), 0.3f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("delayFeedback", 1), "Delay Feedback",
      juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f), 0.4f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("delayMix", 1), "Delay Mix",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  // --- Reverb ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("reverbDecay", 1), "Reverb Decay",
      juce::NormalisableRange<float>(0.0f, 0.99f, 0.01f), 0.7f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("reverbDamping", 1), "Reverb Damping",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("reverbMix", 1), "Reverb Mix",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  return layout;
}

//==============================================================================
void AlgoNebulaProcessor::prepareToPlay(double sampleRate,
                                        int samplesPerBlock) {
  currentSampleRate = sampleRate;
  currentBlockSize = samplesPerBlock;

  // Pre-allocate stereo mix buffer (no allocations on audio thread)
  stereoMixBuffer.setSize(2, samplesPerBlock, false, true, false);

  // Initialize smoothed parameters (20ms ramp time prevents zipper noise)
  masterVolume.reset(sampleRate, 0.02);
  masterVolume.setCurrentAndTargetValue(
      apvts.getRawParameterValue("masterVolume")->load());
  smoothFilterCutoff.reset(sampleRate, 0.02);
  smoothFilterCutoff.setCurrentAndTargetValue(
      apvts.getRawParameterValue("filterCutoff")->load());
  smoothFilterRes.reset(sampleRate, 0.02);
  smoothFilterRes.setCurrentAndTargetValue(
      apvts.getRawParameterValue("filterRes")->load());
  smoothNoiseLevel.reset(sampleRate, 0.02);
  smoothNoiseLevel.setCurrentAndTargetValue(
      apvts.getRawParameterValue("noiseLevel")->load());
  smoothSubLevel.reset(sampleRate, 0.02);
  smoothSubLevel.setCurrentAndTargetValue(
      apvts.getRawParameterValue("subLevel")->load());
  smoothDensityGain.reset(sampleRate, 0.05); // slower ramp for density
  smoothDensityGain.setCurrentAndTargetValue(1.0f);

  // Initialize DSP effects
  chorus.init(static_cast<float>(sampleRate));
  delay.init(static_cast<float>(sampleRate));
  reverb.init(static_cast<float>(sampleRate));

  // Initialize engine with default seed
  engine->randomize(42, 0.3f);
  gridSnapshot.copyFrom(engine->getGrid());
  engineGeneration.store(0, std::memory_order_relaxed);

  // Initialize clock
  clock.reset(sampleRate);
  clock.setBPM(120.0);
  clock.setDivision(ClockDivider::Division::Quarter);

  // Pre-compute tuning table
  tuning.setSystem(Microtuning::System::TwelveTET, 440.0f);

  // Initialize voices
  for (int i = 0; i < kMaxVoices; ++i)
    voices[i].reset();
}

void AlgoNebulaProcessor::releaseResources() {
  // Nothing to release in Phase 1
}

void AlgoNebulaProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                       juce::MidiBuffer &midiMessages) {
  auto startTime = juce::Time::getHighResolutionTicks();

  juce::ScopedNoDenormals noDenormals;

  // Drain UI cell edits into engine grid (bounded)
  cellEditQueue.drainInto(engine->getGridMutable());

  // Process virtual MIDI keyboard input
  keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(),
                                      true);

  // MIDI note-on: key tracking and velocity (no grid reseed — preserves
  // symmetric CA growth patterns from manual seeds)
  for (const auto metadata : midiMessages) {
    const auto msg = metadata.getMessage();
    if (msg.isNoteOn()) {
      // Key tracking: set root key from MIDI note
      int noteKey = msg.getNoteNumber() % 12;
      if (auto *keyParam = apvts.getParameter("key")) {
        keyParam->setValueNotifyingHost(
            keyParam->convertTo0to1(static_cast<float>(noteKey)));
      }

      // Store velocity for voice triggering
      lastMidiVelocity = msg.getFloatVelocity();
    }
  }

  // Read smoothed parameters
  masterVolume.setTargetValue(
      apvts.getRawParameterValue("masterVolume")->load());
  smoothFilterCutoff.setTargetValue(
      apvts.getRawParameterValue("filterCutoff")->load());
  smoothFilterRes.setTargetValue(
      apvts.getRawParameterValue("filterRes")->load());
  smoothNoiseLevel.setTargetValue(
      apvts.getRawParameterValue("noiseLevel")->load());
  smoothSubLevel.setTargetValue(apvts.getRawParameterValue("subLevel")->load());

  const int numSamples = buffer.getNumSamples();

  // --- Read clock params and update clock ---
  float bpm = apvts.getRawParameterValue("bpm")->load();
  int clockDivIdx =
      static_cast<int>(apvts.getRawParameterValue("clockDiv")->load());
  float swing = apvts.getRawParameterValue("swing")->load();
  clock.setBPM(static_cast<double>(bpm));
  clock.setDivision(static_cast<ClockDivider::Division>(clockDivIdx));
  clock.setSwing(static_cast<double>(swing));

  // --- Read algorithm and grid size params, switch engine type ---
  int algoIdx =
      static_cast<int>(apvts.getRawParameterValue("algorithm")->load());
  int gridSizeIdx =
      static_cast<int>(apvts.getRawParameterValue("gridSize")->load());

  // Grid size lookup: rows, cols
  static constexpr int kGridSizes[][2] = {{8, 12},  {12, 16},   {16, 24},
                                          {24, 32}, {32, 48},   {48, 64},
                                          {64, 96}, {128, 128}, {256, 256}};
  int gridRows = kGridSizes[gridSizeIdx][0];
  int gridCols = kGridSizes[gridSizeIdx][1];

  if (algoIdx != lastAlgorithmIdx || gridSizeIdx != lastGridSizeIdx) {
    lastAlgorithmIdx = algoIdx;
    lastGridSizeIdx = gridSizeIdx;
    // Create new engine with updated dimensions
    engine = createEngine(algoIdx, gridRows, gridCols);
    engine->randomize(reseedRng, 0.3f);
    // Release all voices when switching engine
    for (int v = 0; v < kMaxVoices; ++v)
      voices[v].reset();
  }

  // Clear output buffer
  for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    buffer.clear(ch, 0, numSamples);

  // --- Handle transport requests from UI thread ---
  // Seed change from user input
  if (seedChanged.load(std::memory_order_relaxed)) {
    reseedRng = currentSeed.load(std::memory_order_relaxed);
    seedChanged.store(false, std::memory_order_relaxed);
  }

  // Clear grid request
  if (clearRequested.exchange(false, std::memory_order_relaxed)) {
    engine->clear();
    stagnationCounter = 0;
    lastAliveCount = 0;
  }

  // Reseed request (always symmetric)
  if (reseedSymmetricRequested.exchange(false, std::memory_order_relaxed)) {
    reseedRng ^= reseedRng << 13;
    reseedRng ^= reseedRng >> 7;
    reseedRng ^= reseedRng << 17;
    currentSeed.store(reseedRng, std::memory_order_relaxed);
    engine->randomizeSymmetric(reseedRng, 0.3f);
    stagnationCounter = 0;
    lastAliveCount = 0;
  }

  // --- Read symmetry mode ---
  int symmetryIdx =
      static_cast<int>(apvts.getRawParameterValue("symmetry")->load());
  bool useSymmetry = (symmetryIdx == 1);

  // Load factory pattern request
  int patternIdx = loadPatternRequested.exchange(-1, std::memory_order_relaxed);
  if (patternIdx >= 0) {
    FactoryPatternLibrary::applyPattern(engine->getGridMutable(), patternIdx);
    stagnationCounter = 0;
    lastAliveCount = 0;
  }

  // --- Read freeze mode ---
  bool isFrozen =
      static_cast<int>(apvts.getRawParameterValue("freeze")->load()) == 1;

  // Clock-driven engine stepping (only when running and not frozen)
  stepTriggeredThisBlock = false;
  bool isRunning = engineRunning.load(std::memory_order_relaxed);

  // Propagate frozen state to all voices (pauses gate timers)
  for (int v = 0; v < kMaxVoices; ++v)
    voices[v].setFrozen(isFrozen);

  for (int i = 0; i < numSamples; ++i) {
    if (clock.tick() && isRunning && !isFrozen) {
      engine->getGridMutable().snapshotPrev();
      engine->step();
      stepTriggeredThisBlock = true;
    }
  }

  // Auto-reseed: if alive count unchanged for 8 steps, inject cells
  if (stepTriggeredThisBlock) {
    int currentAlive = engine->getGrid().countAlive();
    if (currentAlive == lastAliveCount) {
      ++stagnationCounter;
    } else {
      stagnationCounter = 0;
      lastAliveCount = currentAlive;
    }

    if (stagnationCounter >= 8) {
      auto &grid = engine->getGridMutable();
      const int rows = grid.getRows();
      const int cols = grid.getCols();
      for (int i = 0; i < 5; ++i) {
        reseedRng ^= reseedRng << 13;
        reseedRng ^= reseedRng >> 7;
        reseedRng ^= reseedRng << 17;

        if (useSymmetry) {
          int r = static_cast<int>(reseedRng % ((rows + 1) / 2));
          int c = static_cast<int>((reseedRng >> 16) % ((cols + 1) / 2));
          int mirrorR = rows - 1 - r;
          int mirrorC = cols - 1 - c;
          grid.setCell(r, c, 1);
          grid.setAge(r, c, 1);
          grid.setCell(r, mirrorC, 1);
          grid.setAge(r, mirrorC, 1);
          grid.setCell(mirrorR, c, 1);
          grid.setAge(mirrorR, c, 1);
          grid.setCell(mirrorR, mirrorC, 1);
          grid.setAge(mirrorR, mirrorC, 1);
        } else {
          int r = static_cast<int>(reseedRng % rows);
          int c = static_cast<int>((reseedRng >> 16) % cols);
          grid.setCell(r, c, 1);
          grid.setAge(r, c, 1);
        }
      }
      currentSeed.store(reseedRng, std::memory_order_relaxed);
      stagnationCounter = 0;
    }

    // Overpopulation cap: if grid is >50% full for 3+ steps, reseed sparse
    const int totalCells =
        engine->getGrid().getRows() * engine->getGrid().getCols();
    if (currentAlive > totalCells / 2) {
      ++overpopCounter;
    } else {
      overpopCounter = 0;
    }

    if (overpopCounter >= 3) {
      // Clear and reseed sparsely to break saturation
      reseedRng ^= reseedRng << 13;
      reseedRng ^= reseedRng >> 7;
      reseedRng ^= reseedRng << 17;
      currentSeed.store(reseedRng, std::memory_order_relaxed);
      if (useSymmetry)
        engine->randomizeSymmetric(reseedRng, 0.15f);
      else
        engine->randomize(reseedRng, 0.15f);
      overpopCounter = 0;
      stagnationCounter = 0;
      lastAliveCount = 0;
    }
  }

  // On step: scan grid for active cells, map to notes, trigger voices
  if (stepTriggeredThisBlock) {
    // Read params
    auto waveshapeIdx =
        static_cast<int>(apvts.getRawParameterValue("waveshape")->load());
    auto scaleIdx =
        static_cast<int>(apvts.getRawParameterValue("scale")->load());
    auto keyIdx = static_cast<int>(apvts.getRawParameterValue("key")->load());
    float attack = apvts.getRawParameterValue("attack")->load();
    float hold = apvts.getRawParameterValue("hold")->load();
    float decay = apvts.getRawParameterValue("decay")->load();
    float sustain = apvts.getRawParameterValue("sustain")->load();
    float release = apvts.getRawParameterValue("release")->load();
    float filterCutoff = smoothFilterCutoff.getCurrentValue();
    float filterRes = smoothFilterRes.getCurrentValue();
    int filterModeIdx =
        static_cast<int>(apvts.getRawParameterValue("filterMode")->load());
    float noiseLevel = smoothNoiseLevel.getCurrentValue();
    float subLevel = smoothSubLevel.getCurrentValue();
    int subOctIdx =
        static_cast<int>(apvts.getRawParameterValue("subOctave")->load());
    int maxVoices =
        static_cast<int>(apvts.getRawParameterValue("voiceCount")->load());
    float waveSpread = apvts.getRawParameterValue("waveshapeSpread")->load();
    // Shapes available for cycling (exclude Bell FM = index 7)
    constexpr int kCycleShapeCount = 7;

    quantizer.setScale(static_cast<ScaleQuantizer::Scale>(scaleIdx), keyIdx);

    // --- Density-driven dynamics ---
    const auto &grid = engine->getGrid();
    const int totalCells = grid.getRows() * grid.getCols();
    float density = (totalCells > 0)
                        ? static_cast<float>(grid.countAlive()) / totalCells
                        : 0.0f;
    // Dense grids sound softer; sparse grids are louder
    densityGain = juce::jmap(density, 0.0f, 1.0f, 1.0f, 0.35f);
    smoothDensityGain.setTargetValue(densityGain);
    // Dense grids open the filter; sparse grids keep it tighter
    float densityCutoffMod = juce::jmap(density, 0.0f, 1.0f, 0.5f, 1.0f);
    float modFilterCutoff = filterCutoff * densityCutoffMod;
    // Release voices for cells that just died (no longer retrigger everything)
    for (int v = 0; v < kMaxVoices; ++v) {
      if (voices[v].isActive()) {
        int vRow = voices[v].getGridRow();
        int vCol = voices[v].getGridCol();
        if (vRow >= 0 && vCol >= 0) {
          // Cell died or out of grid bounds: release
          if (grid.getCell(vRow, vCol) == 0) {
            voices[v].noteOff();
          }
        }
      }
    }

    // Trigger new voices only for newly born cells
    int voicesUsed = 0;
    for (int v = 0; v < kMaxVoices; ++v) {
      if (voices[v].isActive())
        ++voicesUsed;
    }

    // Read musicality params
    float noteProb = apvts.getRawParameterValue("noteProbability")->load();
    float velHumanize = apvts.getRawParameterValue("velocityHumanize")->load();
    float melInertia = apvts.getRawParameterValue("melodicInertia")->load();
    float gateTimeFrac = apvts.getRawParameterValue("gateTime")->load();
    float strumSpread = apvts.getRawParameterValue("strumSpread")->load();
    float roundRobin = apvts.getRawParameterValue("roundRobin")->load();

    // Anti-cacophony params
    float consonance = apvts.getRawParameterValue("consonance")->load();
    int maxTrigsPerStep = static_cast<int>(
        apvts.getRawParameterValue("maxTriggersPerStep")->load());
    float restProb = apvts.getRawParameterValue("restProbability")->load();
    float pitchGravity = apvts.getRawParameterValue("pitchGravity")->load();

    // Calculate step interval in samples for gate time
    double stepIntervalSec = clock.getStepIntervalSeconds();
    int stepIntervalSamples =
        static_cast<int>(stepIntervalSec * currentSampleRate);

    // --- Rest probability: chance of skipping ALL triggers this step ---
    if (restProb > 0.0f) {
      musicRng ^= musicRng << 13;
      musicRng ^= musicRng >> 7;
      musicRng ^= musicRng << 17;
      float restRoll = static_cast<float>(musicRng & 0xFFFF) / 65535.0f;
      if (restRoll < restProb)
        goto skipTriggers; // Full rest for this step
    }

    // --- Density-adaptive voice count ---
    {
      float gridDensity = engine->getGrid().getDensity();
      int effectiveMaxVoices = maxVoices;
      if (gridDensity > 0.3f) {
        // Linearly reduce voices: at 100% density, halve the count
        float reduction = (gridDensity - 0.3f) / 0.7f * 0.5f;
        effectiveMaxVoices =
            std::max(1, maxVoices - static_cast<int>(reduction * maxVoices));
      }

      // Collect currently-active MIDI notes for consonance checking
      int activeNotes[kMaxVoices];
      int activeNoteCount = 0;
      for (int v = 0; v < kMaxVoices; ++v) {
        if (voices[v].isActive() && voices[v].getCurrentNote() > 0) {
          activeNotes[activeNoteCount++] = voices[v].getCurrentNote();
        }
      }

      int triggersThisStep = 0;

      for (int col = 0;
           col < grid.getCols() && voicesUsed < effectiveMaxVoices &&
           triggersThisStep < maxTrigsPerStep;
           ++col) {
        for (int row = 0; row < grid.getRows(); ++row) {
          if (engine->cellActivated(row, col)) {
            // --- Note probability: skip trigger randomly ---
            musicRng ^= musicRng << 13;
            musicRng ^= musicRng >> 7;
            musicRng ^= musicRng << 17;
            float roll = static_cast<float>(musicRng & 0xFFFF) / 65535.0f;
            if (roll > noteProb)
              break; // Skip this column entirely

            // --- Melodic inertia: reuse last pitch or compute new ---
            int midiNote;
            musicRng ^= musicRng << 13;
            musicRng ^= musicRng >> 7;
            musicRng ^= musicRng << 17;
            float inertiaRoll =
                static_cast<float>(musicRng & 0xFFFF) / 65535.0f;
            if (inertiaRoll < melInertia && lastTriggeredMidiNote > 0) {
              midiNote = lastTriggeredMidiNote;
            } else if (pitchGravity > 0.0f) {
              // --- Pitch gravity: bias toward chord tones ---
              midiNote = quantizer.quantizeWeighted(
                  row, col, 3, 3, grid.getCols(), pitchGravity, musicRng);
            } else {
              midiNote = quantizer.quantize(row, col, 3, 3, grid.getCols());
            }

            // --- Consonance filter: snap or reject dissonant intervals ---
            if (consonance > 0.0f && activeNoteCount > 0) {
              if (!ScaleQuantizer::isConsonantWithAll(midiNote, activeNotes,
                                                      activeNoteCount)) {
                if (consonance >= 1.0f) {
                  // Hard mode: snap to nearest consonant pitch
                  midiNote = ScaleQuantizer::snapToConsonant(
                      midiNote, activeNotes, activeNoteCount);
                } else {
                  // Probabilistic: snap or allow through
                  float rejectProb = consonance * consonance;
                  musicRng ^= musicRng << 13;
                  musicRng ^= musicRng >> 7;
                  musicRng ^= musicRng << 17;
                  float consRoll =
                      static_cast<float>(musicRng & 0xFFFF) / 65535.0f;
                  if (consRoll < rejectProb) {
                    // Snap instead of reject
                    midiNote = ScaleQuantizer::snapToConsonant(
                        midiNote, activeNotes, activeNoteCount);
                  }
                }
              }
            }

            lastTriggeredMidiNote = midiNote;
            float frequency = tuning.getFrequency(midiNote);

            // --- Velocity humanization + engine intensity ---
            float vel = lastMidiVelocity;

            // Engine-specific intensity modulates velocity
            float cellIntensity = engine->getCellIntensity(row, col);
            vel *= cellIntensity;

            if (velHumanize > 0.0f) {
              musicRng ^= musicRng << 13;
              musicRng ^= musicRng >> 7;
              musicRng ^= musicRng << 17;
              float velOffset =
                  (static_cast<float>(musicRng & 0xFFFF) / 65535.0f - 0.5f) *
                  2.0f * velHumanize;
              vel = std::clamp(vel + velOffset, 0.1f, 1.0f);
            }

            // Find a free voice (round-robin: rotate start index)
            int voiceIdx = -1;
            int searchStart = 0;
            if (roundRobin > 0.0f) {
              musicRng ^= musicRng << 13;
              musicRng ^= musicRng >> 7;
              musicRng ^= musicRng << 17;
              float rrRoll = static_cast<float>(musicRng & 0xFFFF) / 65535.0f;
              if (rrRoll < roundRobin)
                searchStart = roundRobinIndex;
            }
            for (int i = 0; i < kMaxVoices; ++i) {
              int v = (searchStart + i) % kMaxVoices;
              if (!voices[v].isActive()) {
                voiceIdx = v;
                break;
              }
            }
            roundRobinIndex = (roundRobinIndex + 1) % kMaxVoices;
            // Steal quietest if no free voice
            if (voiceIdx < 0) {
              double quietest = 999.0;
              for (int v = 0; v < kMaxVoices; ++v) {
                if (voices[v].getEnvelopeLevel() < quietest) {
                  quietest = voices[v].getEnvelopeLevel();
                  voiceIdx = v;
                }
              }
            }

            if (voiceIdx >= 0) {
              // Waveshape spread: 0 = all voices use selected shape,
              // 1 = cycle through shapes (Bell FM excluded from cycling)
              int shapeIdx = waveshapeIdx;
              if (waveSpread > 0.0f) {
                musicRng ^= musicRng << 13;
                musicRng ^= musicRng >> 7;
                musicRng ^= musicRng << 17;
                float spreadRoll =
                    static_cast<float>(musicRng & 0xFFFF) / 65535.0f;
                if (spreadRoll < waveSpread) {
                  shapeIdx = (waveshapeIdx + col) % kCycleShapeCount;
                }
              }
              auto shape = static_cast<PolyBLEPOscillator::Shape>(shapeIdx);
              voices[voiceIdx].setWaveshape(shape);
              voices[voiceIdx].setEnvelopeParams(attack, hold, decay, sustain,
                                                 release, currentSampleRate);
              voices[voiceIdx].setFilterCutoff(modFilterCutoff);
              voices[voiceIdx].setFilterResonance(filterRes);
              voices[voiceIdx].setFilterMode(
                  static_cast<SVFilter::Mode>(filterModeIdx));
              voices[voiceIdx].setNoiseLevel(noiseLevel);
              voices[voiceIdx].setSubLevel(subLevel);
              voices[voiceIdx].setSubOctave(
                  static_cast<SubOscillator::OctaveMode>(subOctIdx));

              double pan = (grid.getCols() > 1)
                               ? (2.0 * col / (grid.getCols() - 1) - 1.0)
                               : 0.0;
              pan *= static_cast<double>(
                  apvts.getRawParameterValue("stereoWidth")->load());
              voices[voiceIdx].setPan(pan);

              voices[voiceIdx].setGridPosition(row, col);

              // --- Gate time: set per-voice auto-release countdown ---
              if (gateTimeFrac < 1.0f && stepIntervalSamples > 0) {
                int gateSamples =
                    static_cast<int>(gateTimeFrac * stepIntervalSamples);
                if (gateSamples < 1)
                  gateSamples = 1;
                voices[voiceIdx].setGateTime(gateSamples);
              }

              // --- Strum spread: onset delay per column position ---
              if (strumSpread > 0.0f) {
                float colFrac = (grid.getCols() > 1) ? static_cast<float>(col) /
                                                           (grid.getCols() - 1)
                                                     : 0.0f;
                int delaySamples = static_cast<int>(colFrac * strumSpread *
                                                    0.001f * currentSampleRate);
                voices[voiceIdx].setOnsetDelay(delaySamples);
              }

              voices[voiceIdx].noteOn(midiNote, vel, frequency,
                                      currentSampleRate);
              ++voicesUsed;
              ++triggersThisStep;

              // Add to active notes for consonance checking of subsequent
              // triggers
              if (activeNoteCount < kMaxVoices)
                activeNotes[activeNoteCount++] = midiNote;
            }
            break; // One note per column
          }
        }
      }
    }
  skipTriggers:;
  }

  // Render all active voices per-sample
  for (int sample = 0; sample < numSamples; ++sample) {
    double mixL = 0.0;
    double mixR = 0.0;
    for (int v = 0; v < kMaxVoices; ++v) {
      if (voices[v].isActive()) {
        auto stereo = voices[v].renderNextSample();
        mixL += stereo.left;
        mixR += stereo.right;
      }
    }

    // Gain staging: equal-power normalization for summed voices
    const double dGain = static_cast<double>(smoothDensityGain.getNextValue()) /
                         std::sqrt(static_cast<double>(kMaxVoices));
    if (buffer.getNumChannels() >= 2) {
      buffer.setSample(0, sample, static_cast<float>(mixL * dGain));
      buffer.setSample(1, sample, static_cast<float>(mixR * dGain));
    } else if (buffer.getNumChannels() >= 1) {
      buffer.setSample(0, sample,
                       static_cast<float>((mixL + mixR) * 0.5 * dGain));
    }
  }

  // --- Read effect parameters ---
  const float chorusMixP = apvts.getRawParameterValue("chorusMix")->load();
  const float delayMixP = apvts.getRawParameterValue("delayMix")->load();
  const float reverbMixP = apvts.getRawParameterValue("reverbMix")->load();

  // Configure effects (parameter reads, not per-sample)
  chorus.setRate(apvts.getRawParameterValue("chorusRate")->load());
  chorus.setDepth(apvts.getRawParameterValue("chorusDepth")->load());
  chorus.setMix(chorusMixP);
  delay.setTime(apvts.getRawParameterValue("delayTime")->load());
  delay.setFeedback(apvts.getRawParameterValue("delayFeedback")->load());
  delay.setMix(delayMixP);
  reverb.setDecay(apvts.getRawParameterValue("reverbDecay")->load());
  reverb.setDamping(apvts.getRawParameterValue("reverbDamping")->load());
  reverb.setMix(reverbMixP);

  // --- Apply effects chain per-sample (chorus -> delay -> reverb) ---
  if (buffer.getNumChannels() >= 2 &&
      (chorusMixP > 0.0f || delayMixP > 0.0f || reverbMixP > 0.0f)) {
    for (int sample = 0; sample < numSamples; ++sample) {
      float L = buffer.getSample(0, sample);
      float R = buffer.getSample(1, sample);

      // Chorus
      if (chorusMixP > 0.0f) {
        float cL, cR;
        chorus.process(L, R, cL, cR);
        L = cL;
        R = cR;
      }

      // Delay
      if (delayMixP > 0.0f) {
        float dL, dR;
        delay.process(L, R, dL, dR);
        L = dL;
        R = dR;
      }

      // Reverb
      if (reverbMixP > 0.0f) {
        float rL, rR;
        reverb.process(L, R, rL, rR);
        L = rL;
        R = rR;
      }

      // Final NaN/denormal guard
      auto sanitizeOut = [](float x) -> float {
        if (std::isnan(x) || std::isinf(x))
          return 0.0f;
        if (std::fabs(x) < 1.0e-15f)
          return 0.0f;
        return std::max(-4.0f, std::min(4.0f, x));
      };
      L = sanitizeOut(L);
      R = sanitizeOut(R);

      buffer.setSample(0, sample, L);
      buffer.setSample(1, sample, R);
    }
  }

  // Update grid snapshot for GL/UI thread (simple copy, no lock)
  gridSnapshot.copyFrom(engine->getGrid());
  engineGeneration.store(engine->getGeneration(), std::memory_order_relaxed);

  // Apply master volume with smoothing (per-sample)
  for (int sample = 0; sample < numSamples; ++sample) {
    const float vol = masterVolume.getNextValue();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
      buffer.setSample(ch, sample, buffer.getSample(ch, sample) * vol);
  }

  // CPU load measurement
  auto endTime = juce::Time::getHighResolutionTicks();
  double elapsedSeconds =
      juce::Time::highResolutionTicksToSeconds(endTime - startTime);
  double budgetSeconds = static_cast<double>(numSamples) / currentSampleRate;
  cpuLoadPercent.store(
      static_cast<float>(elapsedSeconds / budgetSeconds * 100.0),
      std::memory_order_relaxed);
}

//==============================================================================
juce::AudioProcessorEditor *AlgoNebulaProcessor::createEditor() {
  return new AlgoNebulaEditor(*this);
}

//==============================================================================
void AlgoNebulaProcessor::getStateInformation(juce::MemoryBlock &destData) {
  auto state = apvts.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());

  // Append grid state as child element
  auto *gridXml = xml->createNewChildElement("GridState");
  gridXml->setAttribute("version", 1);
  gridXml->setAttribute("algorithm", lastAlgorithmIdx);
  gridXml->setAttribute("gridSize", lastGridSizeIdx);
  gridXml->setAttribute("seed",
                        juce::String(static_cast<juce::int64>(
                            currentSeed.load(std::memory_order_relaxed))));

  // Encode grid cells and ages as base64
  const auto &grid = engine->getGrid();
  int rows = grid.getRows();
  int cols = grid.getCols();
  gridXml->setAttribute("rows", rows);
  gridXml->setAttribute("cols", cols);

  // Cell data: pack only the active region (rows * cols bytes)
  juce::MemoryBlock cellBlock(static_cast<size_t>(rows * cols));
  for (int r = 0; r < rows; ++r)
    for (int c = 0; c < cols; ++c)
      static_cast<uint8_t *>(cellBlock.getData())[r * cols + c] =
          grid.getCell(r, c);
  gridXml->setAttribute("cells", cellBlock.toBase64Encoding());

  // Age data: pack as little-endian uint16 (rows * cols * 2 bytes)
  juce::MemoryBlock ageBlock(static_cast<size_t>(rows * cols * 2));
  auto *agePtr = static_cast<uint8_t *>(ageBlock.getData());
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      uint16_t age = grid.getAge(r, c);
      int idx = (r * cols + c) * 2;
      agePtr[idx] = static_cast<uint8_t>(age & 0xFF);
      agePtr[idx + 1] = static_cast<uint8_t>((age >> 8) & 0xFF);
    }
  }
  gridXml->setAttribute("ages", ageBlock.toBase64Encoding());

  copyXmlToBinary(*xml, destData);
}

void AlgoNebulaProcessor::setStateInformation(const void *data,
                                              int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
  if (xml == nullptr || !xml->hasTagName(apvts.state.getType()))
    return;

  // Restore APVTS parameters
  apvts.replaceState(juce::ValueTree::fromXml(*xml));

  // Restore grid state if present
  auto *gridXml = xml->getChildByName("GridState");
  if (gridXml == nullptr)
    return; // Legacy state without grid — just use APVTS defaults

  int version = gridXml->getIntAttribute("version", 0);
  if (version < 1)
    return; // Unknown version — skip grid restore

  // Restore engine type and seed
  int algoIdx = gridXml->getIntAttribute("algorithm", 0);
  int gridSizeIdx = gridXml->getIntAttribute("gridSize", 1);
  auto seed = static_cast<uint64_t>(
      gridXml->getStringAttribute("seed", "12345").getLargeIntValue());

  int rows = gridXml->getIntAttribute("rows", 12);
  int cols = gridXml->getIntAttribute("cols", 16);

  // Validate dimensions
  if (rows < 1 || rows > Grid::kMaxRows || cols < 1 || cols > Grid::kMaxCols)
    return;

  // Recreate engine
  lastAlgorithmIdx = algoIdx;
  lastGridSizeIdx = gridSizeIdx;
  engine = createEngine(algoIdx, rows, cols);
  currentSeed.store(seed, std::memory_order_relaxed);

  // Decode cell data
  juce::String cellB64 = gridXml->getStringAttribute("cells", "");
  if (cellB64.isNotEmpty()) {
    juce::MemoryBlock cellBlock;
    if (cellBlock.fromBase64Encoding(cellB64) &&
        cellBlock.getSize() >= static_cast<size_t>(rows * cols)) {
      auto &grid = engine->getGridMutable();
      const auto *cellData = static_cast<const uint8_t *>(cellBlock.getData());
      for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
          grid.setCell(r, c, cellData[r * cols + c]);
    }
  }

  // Decode age data
  juce::String ageB64 = gridXml->getStringAttribute("ages", "");
  if (ageB64.isNotEmpty()) {
    juce::MemoryBlock ageBlock;
    if (ageBlock.fromBase64Encoding(ageB64) &&
        ageBlock.getSize() >= static_cast<size_t>(rows * cols * 2)) {
      auto &grid = engine->getGridMutable();
      const auto *ageData = static_cast<const uint8_t *>(ageBlock.getData());
      for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
          int idx = (r * cols + c) * 2;
          uint16_t age = static_cast<uint16_t>(ageData[idx]) |
                         (static_cast<uint16_t>(ageData[idx + 1]) << 8);
          grid.setAge(r, c, age);
        }
      }
    }
  }

  // Update snapshot for UI
  gridSnapshot.copyFrom(engine->getGrid());
  stagnationCounter = 0;
  lastAliveCount = 0;
}

//==============================================================================
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new AlgoNebulaProcessor();
}
