#pragma once

#include "CellularEngine.h"
#include "Grid.h"
#include <cstdint>
#include <cstring>

/// Particle Swarm: pool of particles with flocking behavior.
/// Internal: float positions + velocities for each particle.
/// Grid projection: trail deposit + decay produces flowing patterns.
class ParticleSwarm final : public CellularEngine {
public:
  static constexpr int kNumParticles = 24;

  struct Particle {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
  };

  explicit ParticleSwarm(int rows = 12, int cols = 16);

  // --- CellularEngine interface ---
  EngineType getType() const override { return EngineType::ParticleSwarm; }
  void step() override;
  void randomize(uint64_t seed, float density) override;
  void randomizeSymmetric(uint64_t seed, float density) override;
  void clear() override;
  const Grid &getGrid() const override { return grid; }
  Grid &getGridMutable() override { return grid; }
  uint64_t getGeneration() const override { return generation; }
  const char *getName() const override { return "Particle Swarm"; }

  // --- Engine-specific intensity ---
  float getCellIntensity(int row, int col) const override {
    float t = trail[row * Grid::kMaxCols + col];
    return (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t);
  }
  bool cellActivated(int row, int col) const override {
    return getGrid().wasBorn(row, col);
  }

  // --- Native data access for visualizer ---
  const Particle *getParticles() const { return particles; }
  const float *getTrailField() const { return trail; }

private:
  void projectToGrid();

  static constexpr int kMax = Grid::kMaxRows * Grid::kMaxCols;
  static constexpr float kTrailDecay = 0.92f;
  static constexpr float kMaxSpeed = 1.5f;
  static constexpr float kFlockWeight = 0.05f;
  static constexpr float kCenterWeight = 0.01f;

  Particle particles[kNumParticles] = {};
  float trail[kMax] = {};
  uint64_t rng = 12345;

  Grid grid;
  uint64_t generation = 0;
  int rows = 12;
  int cols = 16;
};
