#pragma once

#include <cmath>

/// Sub-oscillator: pure sine at -1 or -2 octaves below the voice frequency.
/// RT-safe, no allocations.
class SubOscillator {
public:
  enum class OctaveMode : int { Down1 = 0, Down2, Count };

  void setFrequency(double hz, double sampleRate) {
    baseHz = hz;
    sr = sampleRate;
    recalcIncrement();
  }

  void setOctaveMode(OctaveMode m) {
    octMode = m;
    recalcIncrement();
  }

  void setLevel(double lvl) { level = lvl; }

  double getLevel() const { return level; }
  OctaveMode getOctaveMode() const { return octMode; }

  /// Generate next sub-oscillator sample.
  double nextSample() {
    if (level <= 0.0)
      return 0.0;

    double out = std::sin(kTwoPi * phase) * level;
    phase += phaseIncrement;
    if (phase >= 1.0)
      phase -= 1.0;
    return out;
  }

  /// Reset phase (call on note-on).
  void reset() { phase = 0.0; }

private:
  static constexpr double kTwoPi = 6.283185307179586;

  void recalcIncrement() {
    double hz = baseHz;
    switch (octMode) {
    case OctaveMode::Down1:
      hz *= 0.5;
      break;
    case OctaveMode::Down2:
      hz *= 0.25;
      break;
    default:
      hz *= 0.5;
      break;
    }
    phaseIncrement = sr > 0.0 ? hz / sr : 0.0;
  }

  OctaveMode octMode = OctaveMode::Down1;
  double baseHz = 440.0;
  double sr = 44100.0;
  double phase = 0.0;
  double phaseIncrement = 0.0;
  double level = 0.0;
};
