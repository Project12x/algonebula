#pragma once

#include "CellularEngine.h"
#include "Grid.h"
#include <cstdint>

/// Brian's Brain: 3-state cellular automaton.
/// States: Off(0), On(1), Dying(2).
/// Rule: Dead->On if exactly 2 On neighbors; On->Dying; Dying->Off.
/// Produces flickering chaotic pulses — cells only live 1 generation.
class BriansBrain final : public CellularEngine {
public:
  explicit BriansBrain(int rows = 12, int cols = 16);

  // --- CellularEngine interface ---
  EngineType getType() const override { return EngineType::BriansBrain; }
  void step() override;
  void randomize(uint64_t seed, float density) override;
  void randomizeSymmetric(uint64_t seed, float density) override;
  void clear() override;
  const Grid &getGrid() const override { return grid; }
  Grid &getGridMutable() override { return grid; }
  uint64_t getGeneration() const override { return generation; }
  const char *getName() const override { return "Brian's Brain"; }

private:
  Grid grid;
  Grid scratch;
  uint64_t generation = 0;
};
