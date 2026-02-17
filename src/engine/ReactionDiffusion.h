#pragma once

#include "CellularEngine.h"
#include "Grid.h"
#include <cstdint>
#include <cstring>

/// Reaction-Diffusion: Gray-Scott model on a discrete grid.
/// Internal: two float concentration fields (A, B).
/// Grid projection: B > threshold -> alive; age = B * 255.
/// Produces organic spot and stripe patterns.
class ReactionDiffusion final : public CellularEngine {
public:
  explicit ReactionDiffusion(int rows = 12, int cols = 16);

  // --- CellularEngine interface ---
  EngineType getType() const override { return EngineType::ReactionDiffusion; }
  void step() override;
  void randomize(uint64_t seed, float density) override;
  void randomizeSymmetric(uint64_t seed, float density) override;
  void clear() override;
  const Grid &getGrid() const override { return grid; }
  Grid &getGridMutable() override { return grid; }
  uint64_t getGeneration() const override { return generation; }
  const char *getName() const override { return "Reaction-Diffusion"; }

  // --- Native data access for visualizer ---
  const float *getFieldA() const { return fieldA; }
  const float *getFieldB() const { return fieldB; }

private:
  void projectToGrid(); // Quantize floats to uint8 grid

  // Gray-Scott parameters (tuned for spots)
  static constexpr float kDa = 1.0f;     // Diffusion rate A
  static constexpr float kDb = 0.5f;     // Diffusion rate B
  static constexpr float kFeed = 0.055f; // Feed rate
  static constexpr float kKill = 0.062f; // Kill rate
  static constexpr float kDt = 1.0f;     // Time step
  static constexpr float kThreshold = 0.25f;

  static constexpr int kMax = Grid::kMaxRows * Grid::kMaxCols;

  float fieldA[kMax] = {};
  float fieldB[kMax] = {};
  float scratchA[kMax] = {};
  float scratchB[kMax] = {};

  Grid grid;
  uint64_t generation = 0;
  int rows = 12;
  int cols = 16;
};
