#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AlgoNebulaProcessor::AlgoNebulaProcessor()
    : AudioProcessor(BusesProperties().withOutput(
          "Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "AlgoNebulaState", createParameterLayout()) {}

AlgoNebulaProcessor::~AlgoNebulaProcessor() = default;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
AlgoNebulaProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  // --- Master ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("masterVolume", 1), "Master Volume",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.7f));

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
      juce::ParameterID("voiceCount", 1), "Voice Count", 1, 8, 4));

  // --- Waveshape ---
  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID("waveshape", 1), "Waveshape",
      juce::StringArray{"Sine", "Triangle", "Saw", "Pulse", "Sine+Oct",
                        "Fifth Stack", "Pad", "Bell"},
      0));

  // --- Ambient ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("droneSustain", 1), "Drone Sustain",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("noteProbability", 1), "Note Probability",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

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
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("roundRobin", 1), "Round Robin",
      juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.2f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("velocityHumanize", 1), "Velocity Humanize",
      juce::NormalisableRange<float>(0.0f, 0.3f, 0.01f), 0.05f));

  // --- Envelope ---
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("attack", 1), "Attack",
      juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.5f));

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

  return layout;
}

//==============================================================================
void AlgoNebulaProcessor::prepareToPlay(double sampleRate,
                                        int samplesPerBlock) {
  currentSampleRate = sampleRate;
  currentBlockSize = samplesPerBlock;

  // Pre-allocate stereo mix buffer (no allocations on audio thread)
  stereoMixBuffer.setSize(2, samplesPerBlock, false, true, false);

  // Initialize smoothed parameters
  masterVolume.reset(sampleRate, 0.02); // 20ms smoothing
  masterVolume.setCurrentAndTargetValue(
      apvts.getRawParameterValue("masterVolume")->load());

  // Initialize engine with default seed
  engine.randomize(42, 0.3f);
  gridSnapshot.copyFrom(engine.getGrid());
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
  cellEditQueue.drainInto(engine.getGridMutable());

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

  const int numSamples = buffer.getNumSamples();

  // --- Read clock params and update clock ---
  float bpm = apvts.getRawParameterValue("bpm")->load();
  int clockDivIdx =
      static_cast<int>(apvts.getRawParameterValue("clockDiv")->load());
  float swing = apvts.getRawParameterValue("swing")->load();
  clock.setBPM(static_cast<double>(bpm));
  clock.setDivision(static_cast<ClockDivider::Division>(clockDivIdx));
  clock.setSwing(static_cast<double>(swing));

  // --- Read algorithm param and update rule preset ---
  int algoIdx =
      static_cast<int>(apvts.getRawParameterValue("algorithm")->load());
  if (algoIdx != lastAlgorithmIdx) {
    lastAlgorithmIdx = algoIdx;
    // Map algorithm selector to GoL rule presets
    GameOfLife::RulePreset preset;
    switch (algoIdx) {
    case 0:
      preset = GameOfLife::RulePreset::Classic;
      break;
    case 1:
      preset = GameOfLife::RulePreset::HighLife;
      break;
    case 2:
      preset = GameOfLife::RulePreset::DayAndNight;
      break;
    case 3:
      preset = GameOfLife::RulePreset::Seeds;
      break;
    default:
      preset = GameOfLife::RulePreset::Ambient;
      break;
    }
    engine.setRulePreset(preset);
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
    engine.clear();
    stagnationCounter = 0;
    lastAliveCount = 0;
  }

  // Reseed request (always symmetric)
  if (reseedSymmetricRequested.exchange(false, std::memory_order_relaxed)) {
    reseedRng ^= reseedRng << 13;
    reseedRng ^= reseedRng >> 7;
    reseedRng ^= reseedRng << 17;
    currentSeed.store(reseedRng, std::memory_order_relaxed);
    engine.randomizeSymmetric(reseedRng, 0.3f);
    stagnationCounter = 0;
    lastAliveCount = 0;
  }

  // --- Read symmetry mode ---
  int symmetryIdx =
      static_cast<int>(apvts.getRawParameterValue("symmetry")->load());
  bool useSymmetry = (symmetryIdx == 1);

  // Clock-driven engine stepping (only when running)
  stepTriggeredThisBlock = false;
  bool isRunning = engineRunning.load(std::memory_order_relaxed);
  for (int i = 0; i < numSamples; ++i) {
    if (clock.tick() && isRunning) {
      engine.step();
      stepTriggeredThisBlock = true;
    }
  }

  // Auto-reseed: if alive count unchanged for 8 steps, inject cells
  if (stepTriggeredThisBlock) {
    int currentAlive = engine.getGrid().countAlive();
    if (currentAlive == lastAliveCount) {
      ++stagnationCounter;
    } else {
      stagnationCounter = 0;
      lastAliveCount = currentAlive;
    }

    if (stagnationCounter >= 8) {
      auto &grid = engine.getGridMutable();
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

    // Overpopulation cap: if grid is >70% full for 4+ steps, thin it
    const int totalCells =
        engine.getGrid().getRows() * engine.getGrid().getCols();
    if (currentAlive > totalCells * 7 / 10) {
      ++overpopCounter;
    } else {
      overpopCounter = 0;
    }

    if (overpopCounter >= 4) {
      // Clear and reseed sparsely to break saturation
      reseedRng ^= reseedRng << 13;
      reseedRng ^= reseedRng >> 7;
      reseedRng ^= reseedRng << 17;
      currentSeed.store(reseedRng, std::memory_order_relaxed);
      if (useSymmetry)
        engine.randomizeSymmetric(reseedRng, 0.15f);
      else
        engine.randomize(reseedRng, 0.15f);
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
    float filterCutoff = apvts.getRawParameterValue("filterCutoff")->load();
    float filterRes = apvts.getRawParameterValue("filterRes")->load();
    int filterModeIdx =
        static_cast<int>(apvts.getRawParameterValue("filterMode")->load());
    float noiseLevel = apvts.getRawParameterValue("noiseLevel")->load();
    float subLevel = apvts.getRawParameterValue("subLevel")->load();
    int subOctIdx =
        static_cast<int>(apvts.getRawParameterValue("subOctave")->load());
    int maxVoices =
        static_cast<int>(apvts.getRawParameterValue("voiceCount")->load());
    constexpr int waveCount =
        static_cast<int>(PolyBLEPOscillator::Shape::Count);

    quantizer.setScale(static_cast<ScaleQuantizer::Scale>(scaleIdx), keyIdx);

    // Release all active voices (simple: retrigger each step)
    for (int v = 0; v < kMaxVoices; ++v) {
      if (voices[v].isActive())
        voices[v].noteOff();
    }

    // Scan grid columns for active cells, assign voices
    const auto &grid = engine.getGrid();
    int voicesUsed = 0;

    for (int col = 0; col < grid.getCols() && voicesUsed < maxVoices; ++col) {
      // Find highest active cell in this column
      for (int row = 0; row < grid.getRows(); ++row) {
        if (grid.getCell(row, col) != 0) {
          int midiNote = quantizer.quantize(row, col, 3, 3, grid.getCols());
          float frequency = tuning.getFrequency(midiNote);

          // Find a free voice (or steal quietest)
          int voiceIdx = -1;
          for (int v = 0; v < kMaxVoices; ++v) {
            if (!voices[v].isActive()) {
              voiceIdx = v;
              break;
            }
          }
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
            // Per-column waveshape: base shape + column offset
            int shapeIdx = (waveshapeIdx + col) % waveCount;
            auto shape = static_cast<PolyBLEPOscillator::Shape>(shapeIdx);
            voices[voiceIdx].setWaveshape(shape);
            voices[voiceIdx].setEnvelopeParams(attack, hold, decay, sustain,
                                               release, currentSampleRate);
            voices[voiceIdx].setFilterCutoff(filterCutoff);
            voices[voiceIdx].setFilterResonance(filterRes);
            voices[voiceIdx].setFilterMode(
                static_cast<SVFilter::Mode>(filterModeIdx));
            voices[voiceIdx].setNoiseLevel(noiseLevel);
            voices[voiceIdx].setSubLevel(subLevel);
            voices[voiceIdx].setSubOctave(
                static_cast<SubOscillator::OctaveMode>(subOctIdx));

            // Pan based on column position (-1.0 to 1.0)
            double pan = (grid.getCols() > 1)
                             ? (2.0 * col / (grid.getCols() - 1) - 1.0)
                             : 0.0;
            voices[voiceIdx].setPan(pan);

            voices[voiceIdx].noteOn(midiNote, lastMidiVelocity, frequency,
                                    currentSampleRate);
            ++voicesUsed;
          }
          break; // One note per column
        }
      }
    }
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

    // Gain staging: divide by max voices to prevent clipping
    constexpr double voiceGain = 1.0 / kMaxVoices;
    if (buffer.getNumChannels() >= 2) {
      buffer.setSample(0, sample, static_cast<float>(mixL * voiceGain));
      buffer.setSample(1, sample, static_cast<float>(mixR * voiceGain));
    } else if (buffer.getNumChannels() >= 1) {
      buffer.setSample(0, sample,
                       static_cast<float>((mixL + mixR) * 0.5 * voiceGain));
    }
  }

  // Update grid snapshot for GL/UI thread (simple copy, no lock)
  gridSnapshot.copyFrom(engine.getGrid());
  engineGeneration.store(engine.getGeneration(), std::memory_order_relaxed);

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
  copyXmlToBinary(*xml, destData);
}

void AlgoNebulaProcessor::setStateInformation(const void *data,
                                              int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
  if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

//==============================================================================
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new AlgoNebulaProcessor();
}
