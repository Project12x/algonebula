#pragma once
// Concrete ComputeSimulation adapters for all AlgoNebula CA engines.
// Each adapter provides:
//   - WGSL shader source (embedded as string literal)
//   - Uniform param struct
//   - Initial state generation matching the CPU engine
//
// All adapters inherit GridComputeAdapter for common boilerplate.

#include "GridComputeAdapter.h"
#include <cmath>
#include <cstring>

namespace algonebula {

// ═══════════════════════════════════════════════════════════════════
// Game of Life
// ═══════════════════════════════════════════════════════════════════

class GoLCompute : public GridComputeAdapter {
public:
  struct Params {
    uint32_t width;
    uint32_t height;
    uint32_t birthMask;
    uint32_t survivalMask;
  };

  enum class RulePreset : int {
    Classic = 0, // B3/S23
    HighLife,    // B36/S23
    DayAndNight, // B3678/S34678
    Seeds,       // B2/S
    Ambient,     // B3/S2345
  };

  GoLCompute(int rows, int cols, RulePreset preset = RulePreset::Classic)
      : GridComputeAdapter(rows, cols) {
    setPreset(preset);
  }

  void setPreset(RulePreset preset) {
    switch (preset) {
    case RulePreset::Classic:
      birthMask_ = (1u << 3);
      survivalMask_ = (1u << 2) | (1u << 3);
      break;
    case RulePreset::HighLife:
      birthMask_ = (1u << 3) | (1u << 6);
      survivalMask_ = (1u << 2) | (1u << 3);
      break;
    case RulePreset::DayAndNight:
      birthMask_ = (1u << 3) | (1u << 6) | (1u << 7) | (1u << 8);
      survivalMask_ = (1u << 3) | (1u << 4) | (1u << 6) | (1u << 7) | (1u << 8);
      break;
    case RulePreset::Seeds:
      birthMask_ = (1u << 2);
      survivalMask_ = 0;
      break;
    case RulePreset::Ambient:
      birthMask_ = (1u << 3);
      survivalMask_ = (1u << 2) | (1u << 3) | (1u << 4) | (1u << 5);
      break;
    }
  }

  std::string getShaderSource() const override;
  uint32_t getParamsSize() const override { return sizeof(Params); }
  void writeParams(void *dst) const override {
    Params p{(uint32_t)cols_, (uint32_t)rows_, birthMask_, survivalMask_};
    std::memcpy(dst, &p, sizeof(p));
  }

private:
  uint32_t birthMask_ = (1u << 3);
  uint32_t survivalMask_ = (1u << 2) | (1u << 3);
};

// ═══════════════════════════════════════════════════════════════════
// Brian's Brain
// ═══════════════════════════════════════════════════════════════════

class BriansBrainCompute : public GridComputeAdapter {
public:
  struct Params {
    uint32_t width;
    uint32_t height;
    uint32_t _pad0;
    uint32_t _pad1;
  };

  BriansBrainCompute(int rows, int cols) : GridComputeAdapter(rows, cols) {}

  std::string getShaderSource() const override;
  uint32_t getParamsSize() const override { return sizeof(Params); }
  void writeParams(void *dst) const override {
    Params p{(uint32_t)cols_, (uint32_t)rows_, 0, 0};
    std::memcpy(dst, &p, sizeof(p));
  }

  void generateInitialState(std::vector<float> &state) const override {
    // 30% alive, rest dead (no dying at start)
    uint64_t rng = rngSeed_;
    int densityPct = static_cast<int>(density_ * 100.0f);
    if (densityPct < 1) densityPct = 1;
    if (densityPct > 100) densityPct = 100;
    for (auto &v : state) {
      rng = rng * 6364136223846793005ULL + 1442695040888963407ULL;
      v = ((rng >> 33) % 100 < static_cast<uint64_t>(densityPct)) ? 1.0f : 0.0f;
    }
  }
};

// ═══════════════════════════════════════════════════════════════════
// Cyclic CA
// ═══════════════════════════════════════════════════════════════════

class CyclicCACompute : public GridComputeAdapter {
public:
  struct Params {
    uint32_t width;
    uint32_t height;
    uint32_t numStates;
    uint32_t threshold;
  };

  CyclicCACompute(int rows, int cols, uint32_t states = 16,
                  uint32_t threshold = 1)
      : GridComputeAdapter(rows, cols), numStates_(states),
        threshold_(threshold) {}

  std::string getShaderSource() const override;
  uint32_t getParamsSize() const override { return sizeof(Params); }
  void writeParams(void *dst) const override {
    Params p{(uint32_t)cols_, (uint32_t)rows_, numStates_, threshold_};
    std::memcpy(dst, &p, sizeof(p));
  }

  void generateInitialState(std::vector<float> &state) const override {
    uint64_t rng = rngSeed_;
    for (auto &v : state) {
      rng = rng * 6364136223846793005ULL + 1442695040888963407ULL;
      v = float((rng >> 33) % numStates_) / float(numStates_ - 1);
    }
  }

private:
  uint32_t numStates_ = 16;
  uint32_t threshold_ = 1;
};

// ═══════════════════════════════════════════════════════════════════
// Reaction-Diffusion (Gray-Scott)
// ═══════════════════════════════════════════════════════════════════

class ReactionDiffusionCompute : public GridComputeAdapter {
public:
  struct Params {
    uint32_t width;
    uint32_t height;
    float feed;
    float kill;
    float diffU;
    float diffV;
    float dt;
    uint32_t _pad;
  };

  ReactionDiffusionCompute(int rows, int cols, float feed = 0.055f,
                           float kill = 0.062f)
      : GridComputeAdapter(rows, cols), feed_(feed), kill_(kill) {}

  std::string getShaderSource() const override;
  uint32_t getParamsSize() const override { return sizeof(Params); }
  uint32_t floatsPerCell() const override { return 2; } // u, v interleaved

  void writeParams(void *dst) const override {
    Params p{(uint32_t)cols_, (uint32_t)rows_, feed_, kill_,
             0.21f,           0.105f,          1.0f,  0};
    std::memcpy(dst, &p, sizeof(p));
  }

  void generateInitialState(std::vector<float> &state) const override {
    // U=1 everywhere, V=0 except a central seed square
    for (uint32_t i = 0; i < cellCount(); ++i) {
      state[i * 2] = 1.0f;     // u
      state[i * 2 + 1] = 0.0f; // v
    }
    // Seed: 10x10 square in center
    int cx = rows_ / 2, cy = cols_ / 2;
    for (int r = cx - 5; r < cx + 5; ++r) {
      for (int c = cy - 5; c < cy + 5; ++c) {
        if (r >= 0 && r < rows_ && c >= 0 && c < cols_) {
          int idx = (r * cols_ + c) * 2;
          state[idx] = 0.5f;      // u
          state[idx + 1] = 0.25f; // v
        }
      }
    }
  }

private:
  float feed_ = 0.055f;
  float kill_ = 0.062f;
};

// ═══════════════════════════════════════════════════════════════════
// Lenia 2D
// ═══════════════════════════════════════════════════════════════════

class Lenia2DCompute : public GridComputeAdapter {
public:
  struct Params {
    uint32_t width;
    uint32_t height;
    uint32_t radius;
    uint32_t _pad0;
    float growthMu;
    float growthSigma;
    float dt;
    uint32_t _pad1;
  };

  Lenia2DCompute(int rows, int cols, uint32_t radius = 5)
      : GridComputeAdapter(rows, cols), radius_(radius) {}

  std::string getShaderSource() const override;
  uint32_t getParamsSize() const override { return sizeof(Params); }

  void writeParams(void *dst) const override {
    Params p{(uint32_t)cols_, (uint32_t)rows_, radius_, 0,
             0.15f,           0.045f,          0.1f,    0};
    std::memcpy(dst, &p, sizeof(p));
  }

  void generateInitialState(std::vector<float> &state) const override {
    // Multiple Gaussian blobs for richer initial conditions
    uint64_t rng = rngSeed_;
    int numBlobs = 3 + (rows_ * cols_ > 64 * 64 ? 4 : 0); // more blobs at larger grids
    std::fill(state.begin(), state.end(), 0.0f);
    for (int b = 0; b < numBlobs; ++b) {
      rng = rng * 6364136223846793005ULL + 1442695040888963407ULL;
      int cx = int((rng >> 33) % rows_);
      rng = rng * 6364136223846793005ULL + 1442695040888963407ULL;
      int cy = int((rng >> 33) % cols_);
      float blobRadius = float(std::min(rows_, cols_)) * 0.12f;
      for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
          float dx = float(r - cx);
          float dy = float(c - cy);
          float d2 = (dx * dx + dy * dy) / (blobRadius * blobRadius);
          if (d2 < 4.0f)
            state[r * cols_ + c] = std::min(1.0f, state[r * cols_ + c] + std::exp(-d2));
        }
      }
    }
  }

private:
  uint32_t radius_ = 5;
};

// ═══════════════════════════════════════════════════════════════════
// Particle Swarm
// ═══════════════════════════════════════════════════════════════════

class ParticleSwarmCompute : public GridComputeAdapter {
public:
  struct Params {
    uint32_t width;
    uint32_t height;
    uint32_t numParticles;
    uint32_t _pad;
    float trailDecay;
    float socialWeight;
    float inertia;
    float trailDeposit;
  };

  ParticleSwarmCompute(int rows, int cols, uint32_t particles = 256)
      : GridComputeAdapter(rows, cols), numParticles_(particles) {}

  std::string getShaderSource() const override;
  uint32_t getParamsSize() const override { return sizeof(Params); }

  void writeParams(void *dst) const override {
    Params p{(uint32_t)cols_,
             (uint32_t)rows_,
             numParticles_,
             0,
             0.95f,
             0.3f,
             0.9f,
             0.5f};
    std::memcpy(dst, &p, sizeof(p));
  }

  // Particle Swarm needs the "decayTrails" entry point for the grid pass
  const char *getEntryPoint() const override { return "decayTrails"; }
  // Secondary pass moves particles and deposits trails
  const char *getSecondaryEntryPoint() const override {
    return "moveParticles";
  }
  void getSecondaryDispatch(uint32_t &x, uint32_t &y,
                            uint32_t &z) const override {
    x = (numParticles_ + 63) / 64; // workgroup_size(64)
    y = 1;
    z = 1;
  }

  uint32_t extraBufferCount() const override { return 1; }
  uint64_t extraBufferSize(uint32_t) const override {
    return numParticles_ * 4 * sizeof(float); // x, y, vx, vy per particle
  }

  void initExtraBuffers(WGPUDevice device, WGPUQueue queue) override {
    // Random initial positions
    std::vector<float> data(numParticles_ * 4);
    uint64_t rng = 12345;
    for (uint32_t i = 0; i < numParticles_; ++i) {
      rng = rng * 6364136223846793005ULL + 1442695040888963407ULL;
      data[i * 4] = float((rng >> 33) % cols_);
      rng = rng * 6364136223846793005ULL + 1442695040888963407ULL;
      data[i * 4 + 1] = float((rng >> 33) % rows_);
      data[i * 4 + 2] = 0.0f;
      data[i * 4 + 3] = 0.0f;
    }
    uploadBuffer(queue, extraBuffers_[0], data.data(),
                 data.size() * sizeof(float));
  }

  void generateInitialState(std::vector<float> &state) const override {
    // Start with empty trail grid
    std::fill(state.begin(), state.end(), 0.0f);
  }

private:
  uint32_t numParticles_ = 256;
};

// ═══════════════════════════════════════════════════════════════════
// Brownian Field
// ═══════════════════════════════════════════════════════════════════

class BrownianFieldCompute : public GridComputeAdapter {
public:
  struct Params {
    uint32_t width;
    uint32_t height;
    uint32_t numWalkers;
    uint32_t stepCount;
    float diffusionRate;
    float decayRate;
    float deposit;
    uint32_t _pad;
  };

  BrownianFieldCompute(int rows, int cols, uint32_t walkers = 200)
      : GridComputeAdapter(rows, cols), numWalkers_(walkers) {}

  std::string getShaderSource() const override;
  uint32_t getParamsSize() const override { return sizeof(Params); }

  void writeParams(void *dst) const override {
    Params p{(uint32_t)cols_,
             (uint32_t)rows_,
             numWalkers_,
             frame_,
             0.1f,
             0.98f,
             0.3f,
             0};
    std::memcpy(dst, &p, sizeof(p));
  }

  const char *getEntryPoint() const override { return "diffuseDecay"; }
  // Secondary pass does random walk + energy deposit
  const char *getSecondaryEntryPoint() const override {
    return "walkDeposit";
  }
  void getSecondaryDispatch(uint32_t &x, uint32_t &y,
                            uint32_t &z) const override {
    x = (numWalkers_ + 63) / 64; // workgroup_size(64)
    y = 1;
    z = 1;
  }

  uint32_t extraBufferCount() const override { return 1; }
  uint64_t extraBufferSize(uint32_t) const override {
    return numWalkers_ * 2 * sizeof(float); // x, y per walker
  }

  void initExtraBuffers(WGPUDevice device, WGPUQueue queue) override {
    std::vector<float> data(numWalkers_ * 2);
    uint64_t rng = 7777;
    for (uint32_t i = 0; i < numWalkers_; ++i) {
      rng = rng * 6364136223846793005ULL + 1442695040888963407ULL;
      data[i * 2] = float((rng >> 33) % cols_);
      rng = rng * 6364136223846793005ULL + 1442695040888963407ULL;
      data[i * 2 + 1] = float((rng >> 33) % rows_);
    }
    uploadBuffer(queue, extraBuffers_[0], data.data(),
                 data.size() * sizeof(float));
  }

  void generateInitialState(std::vector<float> &state) const override {
    std::fill(state.begin(), state.end(), 0.0f);
  }

private:
  uint32_t numWalkers_ = 200;
};

} // namespace algonebula
