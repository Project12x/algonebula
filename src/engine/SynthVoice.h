#pragma once

#include "AHDSREnvelope.h"
#include "NoiseLayer.h"
#include "PolyBLEPOscillator.h"
#include "SVFilter.h"
#include "SubOscillator.h"

/// Composite synth voice: Oscillator -> [+ Sub + Noise] -> Filter -> Envelope.
/// Outputs stereo (L, R) with per-voice panning.
/// All state internal, no allocations. RT-safe.
class SynthVoice {
public:
  struct StereoSample {
    double left = 0.0;
    double right = 0.0;
  };

  /// Trigger a note. midiNote 0-127, velocity 0.0-1.0.
  void noteOn(int midiNote, double velocity, double frequencyHz,
              double sampleRate) {
    currentNote = midiNote;
    vel = velocity;
    active = true;
    sr = sampleRate;

    osc.reset();
    osc.setFrequency(frequencyHz, sampleRate);

    sub.reset();
    sub.setFrequency(frequencyHz, sampleRate);

    filter.reset();
    filter.setCutoff(filterCutoffHz, sampleRate);

    envelope.noteOn();

    // Reset onset delay (strum spread)
    onsetDelaySamples = pendingOnsetDelay;
    pendingOnsetDelay = 0;
  }

  /// Release the note.
  void noteOff() { envelope.noteOff(); }

  /// Render one stereo sample.
  StereoSample renderNextSample() {
    if (!active)
      return {0.0, 0.0};

    // Onset delay (strum spread): count down silently
    if (onsetDelaySamples > 0) {
      --onsetDelaySamples;
      return {0.0, 0.0};
    }

    // Gate timer: release note when countdown expires (paused when frozen)
    if (gateRemainingSamples > 0 && !frozen) {
      --gateRemainingSamples;
      if (gateRemainingSamples == 0) {
        envelope.noteOff();
      }
    }

    double envLevel = envelope.nextSample();
    if (!envelope.isActive()) {
      active = false;
      return {0.0, 0.0};
    }

    // Generate oscillator
    double oscOut = osc.nextSample();

    // Add sub-oscillator
    double subOut = sub.nextSample();

    // Add noise
    double noiseOut = noise.nextSample();

    // Mix before filter
    double mixed = oscOut + subOut + noiseOut;

    // Filter
    double filtered = filter.process(mixed);

    // Apply envelope and velocity
    double output = filtered * envLevel * vel;

    // Pan: equal-power panning
    // pan = 0.0 (center), -1.0 (full left), 1.0 (full right)
    double panAngle = (pan + 1.0) * 0.5;                    // 0.0 to 1.0
    double leftGain = std::cos(panAngle * 1.5707963267949); // pi/4
    double rightGain = std::sin(panAngle * 1.5707963267949);

    return {output * leftGain, output * rightGain};
  }

  /// Check if voice is still producing output.
  bool isActive() const { return active; }

  /// Get the MIDI note this voice is playing.
  int getCurrentNote() const { return currentNote; }

  /// Grid position this voice was triggered from (-1 = not grid-assigned).
  int getGridRow() const { return gridRow; }
  int getGridCol() const { return gridCol; }
  void setGridPosition(int row, int col) {
    gridRow = row;
    gridCol = col;
  }

  /// Get the envelope level (for voice stealing comparison).
  double getEnvelopeLevel() const { return envelope.getLevel(); }

  // --- Configuration ---

  void setWaveshape(PolyBLEPOscillator::Shape s) { osc.setWaveshape(s); }

  void setPulseWidth(double pw) { osc.setPulseWidth(pw); }

  void setEnvelopeParams(double attack, double hold, double decay,
                         double sustain, double release, double sampleRate) {
    envelope.setParameters(attack, hold, decay, sustain, release, sampleRate);
  }

  void setFilterCutoff(double hz) {
    filterCutoffHz = hz;
    filter.setCutoff(hz, sr);
  }

  void setFilterResonance(double res) { filter.setResonance(res); }

  void setFilterMode(SVFilter::Mode m) { filter.setMode(m); }

  void setNoiseLevel(double lvl) { noise.setLevel(lvl); }

  void setSubLevel(double lvl) { sub.setLevel(lvl); }

  void setSubOctave(SubOscillator::OctaveMode m) { sub.setOctaveMode(m); }

  void setPan(double p) { pan = p; } // -1.0 to 1.0

  /// Set frozen state: pauses gate timer so voices sustain indefinitely.
  void setFrozen(bool f) { frozen = f; }

  /// Set gate time in samples. 0 = hold until cell dies (no auto-release).
  void setGateTime(int samples) { gateRemainingSamples = samples; }

  /// Set onset delay in samples (for strum spread). Must be set before noteOn.
  void setOnsetDelay(int samples) { pendingOnsetDelay = samples; }

  /// Reset voice to idle state.
  void reset() {
    active = false;
    currentNote = -1;
    vel = 0.0;
    gridRow = -1;
    gridCol = -1;
    gateRemainingSamples = 0;
    onsetDelaySamples = 0;
    pendingOnsetDelay = 0;
    osc.reset();
    sub.reset();
    filter.reset();
    envelope.reset();
    noise.reset();
  }

private:
  bool active = false;
  int currentNote = -1;
  double vel = 0.0;
  double pan = 0.0; // -1.0 (L) to 1.0 (R)
  double sr = 44100.0;
  int gridRow = -1; // Grid cell this voice was triggered from
  int gridCol = -1;
  double filterCutoffHz = 8000.0;
  bool frozen = false;
  int gateRemainingSamples = 0; // 0 = hold until cell dies
  int onsetDelaySamples = 0;    // Countdown before audio output starts
  int pendingOnsetDelay = 0;    // Set before noteOn, applied on noteOn

  PolyBLEPOscillator osc;
  AHDSREnvelope envelope;
  SVFilter filter;
  NoiseLayer noise;
  SubOscillator sub;
};
