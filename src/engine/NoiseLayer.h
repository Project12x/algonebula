#pragma once

#include <cstdint>

/// Lightweight white noise generator for texture layering.
/// Uses xorshift64 PRNG for RT-safe, allocation-free noise.
class NoiseLayer {
public:
  /// Set noise output level (0.0 = silent, 1.0 = full).
  void setLevel(double lvl) { level = lvl; }

  /// Get current level.
  double getLevel() const { return level; }

  /// Reset PRNG state.
  void reset(uint64_t seed = 12345ULL) { state = seed != 0 ? seed : 1ULL; }

  /// Generate next noise sample, scaled by level.
  double nextSample() {
    if (level <= 0.0)
      return 0.0;

    // xorshift64
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;

    // Convert to [-1, 1] range
    double normalized = static_cast<double>(static_cast<int64_t>(state)) /
                        9223372036854775807.0;
    return normalized * level;
  }

private:
  double level = 0.0;
  uint64_t state = 12345ULL;
};
