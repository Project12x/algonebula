#pragma once

#include "Grid.h"
#include <cstdint>

/// Engine type identifier for safe downcasting by visualizers.
enum class EngineType {
  GoL,
  BriansBrain,
  CyclicCA,
  ReactionDiffusion,
  ParticleSwarm,
  Lenia,
  BrownianField
};

/// Abstract interface for all cellular automata engines.
/// All implementations must be allocation-free in step() — all memory
/// pre-allocated in reset() or constructor.
class CellularEngine {
public:
  virtual ~CellularEngine() = default;

  /// Get engine type for safe downcasting.
  virtual EngineType getType() const = 0;

  /// Advance the automaton by one generation.
  /// Must be O(rows * cols), no allocations.
  virtual void step() = 0;

  /// Reset the grid to initial state using given seed and density.
  /// @param seed  Deterministic PRNG seed for reproducible patterns.
  /// @param density  Fill ratio [0, 1] for random initialization.
  virtual void randomize(uint64_t seed, float density) = 0;

  /// Reset with 4-fold mirror symmetry (random in one quadrant, mirrored).
  /// Produces visually striking symmetric growth patterns.
  /// @param seed  Deterministic PRNG seed.
  /// @param density  Fill ratio [0, 1] for the source quadrant.
  virtual void randomizeSymmetric(uint64_t seed, float density) = 0;

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

  /// Get continuous cell intensity [0.0, 1.0] for engine-specific triggering.
  /// Binary engines return 0.0 or 1.0. Continuous engines return native float.
  virtual float getCellIntensity(int row, int col) const {
    return getGrid().getCell(row, col) ? 1.0f : 0.0f;
  }

  /// Check if a cell was "activated" this step (newly triggered).
  /// Binary engines use wasBorn(). Continuous engines detect threshold
  /// crossing.
  virtual bool cellActivated(int row, int col) const {
    return getGrid().wasBorn(row, col);
  }
};
