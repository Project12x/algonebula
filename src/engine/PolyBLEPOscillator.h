#pragma once

#include <cmath>
#include <cstdint>

/// PolyBLEP anti-aliased oscillator with 8 waveshapes.
/// All methods are RT-safe — no allocations, no branching on shape per-sample
/// beyond a switch.  Uses double-precision phase accumulator.
class PolyBLEPOscillator {
public:
  enum class Shape : int {
    Sine = 0,
    Triangle,
    Saw,
    Pulse,
    SineOct,    // sin + 0.5*sin(2x)
    FifthStack, // sin + 0.5*sin(1.5x)
    Pad,        // 2x detuned saws
    Bell,       // 2-op FM
    Count
  };

  void setFrequency(double hz, double sampleRate) {
    phaseIncrement = hz / sampleRate;
    // Pad: two detuned saws (+-7 cents)
    double detuneFactor = std::pow(2.0, 7.0 / 1200.0); // 7 cents
    padIncrementA = (hz * detuneFactor) / sampleRate;
    padIncrementB = (hz / detuneFactor) / sampleRate;
    // Bell: modulator at non-integer ratio for inharmonic spectrum
    bellModIncrement = (hz * 1.4) / sampleRate;
  }

  void setWaveshape(Shape s) { shape = s; }
  void setPulseWidth(double pw) { pulseWidth = pw; }
  void setFMIndex(double idx) { fmIndex = idx; }

  Shape getWaveshape() const { return shape; }

  /// Reset phase to 0 (call on note-on to avoid click)
  void reset() {
    phase = 0.0;
    padPhaseA = 0.0;
    padPhaseB = 0.0;
    bellModPhase = 0.0;
    triIntegrator = 0.0;
  }

  /// Generate next sample. Call once per sample.
  double nextSample() {
    double out = 0.0;

    switch (shape) {
    case Shape::Sine:
      out = std::sin(kTwoPi * phase);
      break;

    case Shape::Triangle:
      out = generateTriangle();
      break;

    case Shape::Saw:
      out = generateSaw(phase, phaseIncrement);
      break;

    case Shape::Pulse:
      out = generatePulse();
      break;

    case Shape::SineOct:
      out = std::sin(kTwoPi * phase) + 0.5 * std::sin(kTwoPi * 2.0 * phase);
      out *= 0.667; // normalize
      break;

    case Shape::FifthStack:
      out = std::sin(kTwoPi * phase) + 0.5 * std::sin(kTwoPi * 1.5 * phase);
      out *= 0.667;
      break;

    case Shape::Pad:
      out = generatePad();
      break;

    case Shape::Bell:
      out = generateBell();
      break;

    default:
      break;
    }

    advancePhase(phase, phaseIncrement);
    if (shape == Shape::Pad) {
      advancePhase(padPhaseA, padIncrementA);
      advancePhase(padPhaseB, padIncrementB);
    }
    if (shape == Shape::Bell) {
      advancePhase(bellModPhase, bellModIncrement);
    }

    return out;
  }

private:
  static constexpr double kTwoPi = 6.283185307179586;

  // --- PolyBLEP residual ---
  // Polynomial bandlimited step at discontinuity.
  // t: normalized distance from discontinuity [0,1) in one direction.
  static double polyBLEP(double t, double dt) {
    if (t < dt) {
      // Just after discontinuity
      double tn = t / dt;
      return tn + tn - tn * tn - 1.0;
    }
    if (t > 1.0 - dt) {
      // Just before discontinuity
      double tn = (t - 1.0) / dt;
      return tn * tn + tn + tn + 1.0;
    }
    return 0.0;
  }

  // --- Saw: naive ramp [-1,1] + PolyBLEP correction ---
  double generateSaw(double ph, double dt) const {
    double naiveSaw = 2.0 * ph - 1.0;
    return naiveSaw - polyBLEP(ph, dt);
  }

  // --- Pulse: two offset saws subtracted ---
  double generatePulse() const {
    double saw1 = generateSaw(phase, phaseIncrement);
    double shiftedPhase = phase + pulseWidth;
    if (shiftedPhase >= 1.0)
      shiftedPhase -= 1.0;
    double saw2 = generateSaw(shiftedPhase, phaseIncrement);
    // Pulse = saw1 - saw2, normalized
    return (saw1 - saw2) * 0.5;
  }

  // --- Triangle: integrated square + leaky integrator ---
  double generateTriangle() {
    // Generate a square wave with PolyBLEP
    double sq = (phase < 0.5) ? 1.0 : -1.0;
    sq += polyBLEP(phase, phaseIncrement);
    double shiftedPhase = phase + 0.5;
    if (shiftedPhase >= 1.0)
      shiftedPhase -= 1.0;
    sq -= polyBLEP(shiftedPhase, phaseIncrement);

    // Leaky integrator to smooth square -> triangle
    // 4.0 * phaseIncrement scales the integrator to unit amplitude
    triIntegrator += 4.0 * phaseIncrement * sq;
    // Leaky factor prevents DC drift
    triIntegrator *= 0.999;
    return triIntegrator;
  }

  // --- Pad: two detuned saws summed ---
  double generatePad() const {
    double sawA = generateSaw(padPhaseA, padIncrementA);
    double sawB = generateSaw(padPhaseB, padIncrementB);
    return (sawA + sawB) * 0.5;
  }

  // --- Bell: 2-operator FM ---
  double generateBell() const {
    double mod = std::sin(kTwoPi * bellModPhase);
    return std::sin(kTwoPi * phase + fmIndex * mod);
  }

  static void advancePhase(double &ph, double inc) {
    ph += inc;
    if (ph >= 1.0)
      ph -= 1.0;
  }

  Shape shape = Shape::Sine;
  double phase = 0.0;
  double phaseIncrement = 0.0;
  double pulseWidth = 0.5;

  // Pad phases
  double padPhaseA = 0.0;
  double padPhaseB = 0.0;
  double padIncrementA = 0.0;
  double padIncrementB = 0.0;

  // Bell FM
  double bellModPhase = 0.0;
  double bellModIncrement = 0.0;
  double fmIndex = 5.0; // modulation index

  // Triangle integrator state
  double triIntegrator = 0.0;
};
