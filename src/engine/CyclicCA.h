#pragma once

#include "CellularEngine.h"
#include "Grid.h"
#include <cstdint>

/// Cyclic Cellular Automaton: N-state predator/prey system.
/// A cell advances to (state+1)%N if any Moore neighbor is at (state+1)%N.
/// Produces expanding spiral waves with color variation per state.
class CyclicCA final : public CellularEngine {
public:
  static constexpr int kNumStates = 6;

  explicit CyclicCA(int rows = 12, int cols = 16);

  // --- CellularEngine interface ---
  EngineType getType() const override { return EngineType::CyclicCA; }
  void step() override;
  void randomize(uint64_t seed, float density) override;
  void randomizeSymmetric(uint64_t seed, float density) override;
  void clear() override;
  const Grid &getGrid() const override { return grid; }
  Grid &getGridMutable() override { return grid; }
  uint64_t getGeneration() const override { return generation; }
  const char *getName() const override { return "Cyclic CA"; }

private:
  Grid grid;
  Grid scratch;
  uint64_t generation = 0;
};
