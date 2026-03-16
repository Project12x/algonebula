#pragma once
#include "CellularEngine.h"

/// CPU stub for 3D Lenia — actual simulation runs on GPU.
/// step() is a no-op. Grid is populated by GPU readback (MIP projection).
class Lenia3DStub : public CellularEngine {
public:
  Lenia3DStub(int rows, int cols) : grid_(rows, cols) { grid_.clear(); }

  EngineType getType() const override { return EngineType::Lenia3D; }
  void step() override {} // GPU does the work
  void randomize(uint64_t, float) override { grid_.clear(); }
  void randomizeSymmetric(uint64_t, float) override { grid_.clear(); }
  void clear() override { grid_.clear(); }
  const Grid &getGrid() const override { return grid_; }
  Grid &getGridMutable() override { return grid_; }
  uint64_t getGeneration() const override { return 0; }
  const char *getName() const override { return "Lenia 3D"; }
  float getGainScale() const override { return 0.6f; }

private:
  Grid grid_;
};
