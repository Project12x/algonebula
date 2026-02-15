#pragma once

#include "Grid.h"
#include <cstdint>

/// Abstract interface for all cellular automata engines.
/// All implementations must be allocation-free in step() — all memory
/// pre-allocated in reset() or constructor.
class CellularEngine {
public:
  virtual ~CellularEngine() = default;

  /// Advance the automaton by one generation.
  /// Must be O(rows * cols), no allocations.
  virtual void step() = 0;

  /// Reset the grid to initial state using given seed and density.
  /// @param seed  Deterministic PRNG seed for reproducible patterns.
  /// @param density  Fill ratio [0, 1] for random initialization.
  virtual void randomize(uint64_t seed, float density) = 0;

  /// Reset to a completely empty grid.
  virtual void clear() = 0;

  /// Get read-only reference to current grid state.
  virtual const Grid &getGrid() const = 0;

  /// Get mutable reference (for UI cell edits queued via SPSC).
  virtual Grid &getGridMutable() = 0;

  /// Get the current generation count.
  virtual uint64_t getGeneration() const = 0;

  /// Get algorithm name for display.
  virtual const char *getName() const = 0;
};
