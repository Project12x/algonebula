#pragma once

#include "CellularEngine.h"
#include "Grid.h"
#include <cstdint>
#include <cstring>

/// Brownian Field: random walkers depositing energy on a decaying field.
/// Internal: walker positions (float) + energy field (float).
/// Grid projection: energy > threshold -> alive; age = energy * 255.
/// Produces diffuse, slowly-shifting noise patterns.
class BrownianField final : public CellularEngine {
public:
  static constexpr int kNumWalkers = 32;

  struct Walker {
    float x = 0.0f;
    float y = 0.0f;
  };

  explicit BrownianField(int rows = 12, int cols = 16);

  // --- CellularEngine interface ---
  EngineType getType() const override { return EngineType::BrownianField; }
  void step() override;
  void randomize(uint64_t seed, float density) override;
  void randomizeSymmetric(uint64_t seed, float density) override;
  void clear() override;
  const Grid &getGrid() const override { return grid; }
  Grid &getGridMutable() override { return grid; }
  uint64_t getGeneration() const override { return generation; }
  const char *getName() const override { return "Brownian Field"; }

  // --- Native data access for visualizer ---
  const Walker *getWalkers() const { return walkers; }
  const float *getEnergyField() const { return energy; }

private:
  void projectToGrid();

  static constexpr int kMax = Grid::kMaxRows * Grid::kMaxCols;
  static constexpr float kEnergyDecay = 0.95f;
  static constexpr float kDepositAmount = 0.8f;
  static constexpr float kThreshold = 0.1f;

  Walker walkers[kNumWalkers] = {};
  float energy[kMax] = {};
  uint64_t rng = 12345;

  Grid grid;
  uint64_t generation = 0;
  int rows = 12;
  int cols = 16;
};
