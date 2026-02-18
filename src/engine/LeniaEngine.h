#pragma once

#include "CellularEngine.h"
#include "Grid.h"
#include <cstdint>
#include <vector>

/// Lenia: Continuous-neighborhood cellular automaton.
/// Internal: float state field (0.0-1.0) with wide bell-curve kernel
/// convolution (radius 3) and Gaussian growth function.
/// Produces smooth organic blob patterns that move and morph.
class LeniaEngine final : public CellularEngine {
public:
  explicit LeniaEngine(int rows = 12, int cols = 16);

  // --- CellularEngine interface ---
  EngineType getType() const override { return EngineType::Lenia; }
  void step() override;
  void randomize(uint64_t seed, float density) override;
  void randomizeSymmetric(uint64_t seed, float density) override;
  void clear() override;
  const Grid &getGrid() const override { return grid; }
  Grid &getGridMutable() override { return grid; }
  uint64_t getGeneration() const override { return generation; }
  const char *getName() const override { return "Lenia"; }

  // --- Engine-specific intensity ---
  float getCellIntensity(int row, int col) const override {
    return stateField[row * Grid::kMaxCols + col];
  }
  bool cellActivated(int row, int col) const override {
    // For continuous: use grid's wasBorn (threshold crossing projected)
    return getGrid().wasBorn(row, col);
  }

  // --- Native data access for visualizer ---
  const float *getStateField() const { return stateField.data(); }

private:
  void projectToGrid();
  void precomputeKernel();

  // Lenia parameters
  static constexpr int kRadius = 3;
  static constexpr float kMu = 0.15f;     // Growth center
  static constexpr float kSigma = 0.015f; // Growth width
  static constexpr float kDt = 0.1f;      // Time step
  static constexpr float kThreshold = 0.1f;

  static constexpr int kMax = Grid::kMaxRows * Grid::kMaxCols;
  static constexpr int kKernelSize = (2 * kRadius + 1) * (2 * kRadius + 1);

  std::vector<float> stateField;
  std::vector<float> scratch;
  float kernel[kKernelSize] = {};
  float kernelSum = 0.0f;

  Grid grid;
  uint64_t generation = 0;
  int rows = 12;
  int cols = 16;
};
