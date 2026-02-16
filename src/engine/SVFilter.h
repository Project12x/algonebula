#pragma once

#include <cmath>

/// State-variable filter (Cytomic/Simper topology).
/// 4 modes: LP, HP, BP, Notch. RT-safe, no allocations.
/// Coefficients recomputed only on parameter change.
class SVFilter {
public:
  enum class Mode : int { LowPass = 0, HighPass, BandPass, Notch, Count };

  /// Update filter coefficients. Call when cutoff or resonance changes.
  void setCutoff(double cutoffHz, double sampleRate) {
    cutoff = cutoffHz;
    sr = sampleRate;
    updateCoefficients();
  }

  void setResonance(double res) {
    resonance = res;
    updateCoefficients();
  }

  void setMode(Mode m) { mode = m; }

  Mode getMode() const { return mode; }

  /// Process a single sample. Returns filtered output.
  double process(double input) {
    // Cytomic SVF: direct form
    double v3 = input - ic2eq;
    double v1 = a1 * ic1eq + a2 * v3;
    double v2 = ic2eq + a2 * ic1eq + a3 * v3;
    ic1eq = 2.0 * v1 - ic1eq;
    ic2eq = 2.0 * v2 - ic2eq;

    switch (mode) {
    case Mode::LowPass:
      return v2;
    case Mode::HighPass:
      return input - k * v1 - v2;
    case Mode::BandPass:
      return v1;
    case Mode::Notch:
      return input - k * v1;
    default:
      return v2;
    }
  }

  /// Reset internal state (call on note-on).
  void reset() {
    ic1eq = 0.0;
    ic2eq = 0.0;
  }

private:
  void updateCoefficients() {
    // Clamp cutoff to safe range
    double fc = cutoff;
    if (fc < 20.0)
      fc = 20.0;
    double nyquist = sr * 0.5;
    if (fc > nyquist * 0.95)
      fc = nyquist * 0.95;

    double g = std::tan(3.14159265358979 * fc / sr);
    // k controls resonance: k = 2 - 2*res gives Q from 0.5 to infinity
    // Clamp resonance to avoid instability
    double res = resonance;
    if (res < 0.0)
      res = 0.0;
    if (res > 0.99)
      res = 0.99;
    k = 2.0 * (1.0 - res);

    a1 = 1.0 / (1.0 + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;
  }

  Mode mode = Mode::LowPass;
  double cutoff = 1000.0;
  double resonance = 0.0;
  double sr = 44100.0;

  // Coefficients
  double k = 2.0;
  double a1 = 0.0;
  double a2 = 0.0;
  double a3 = 0.0;

  // State
  double ic1eq = 0.0;
  double ic2eq = 0.0;
};
