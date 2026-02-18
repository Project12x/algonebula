#include "LeniaEngine.h"
#include <algorithm>
#include <cmath>

namespace {
uint64_t xorshift64(uint64_t &state) {
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}
} // namespace

LeniaEngine::LeniaEngine(int r, int c)
    : stateField(Grid::kMaxCells, 0.0f), scratch(Grid::kMaxCells, 0.0f),
      grid(r, c), rows(r), cols(c) {
  precomputeKernel();
}

void LeniaEngine::precomputeKernel() {
  // Bell-curve kernel: weight decreases with distance from center
  kernelSum = 0.0f;
  int idx = 0;
  for (int dr = -kRadius; dr <= kRadius; ++dr) {
    for (int dc = -kRadius; dc <= kRadius; ++dc) {
      float dist = std::sqrt(static_cast<float>(dr * dr + dc * dc));
      float normalized = dist / static_cast<float>(kRadius);
      // Bell curve: exp(-0.5 * ((r - 0.5) / 0.15)^2)
      float diff = normalized - 0.5f;
      kernel[idx] = std::exp(-0.5f * diff * diff / (0.15f * 0.15f));
      kernelSum += kernel[idx];
      ++idx;
    }
  }
}

void LeniaEngine::step() {
  // Kernel convolution: weighted neighborhood sum
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      float neighborSum = 0.0f;
      int kidx = 0;
      for (int dr = -kRadius; dr <= kRadius; ++dr) {
        for (int dc = -kRadius; dc <= kRadius; ++dc) {
          int nr = ((r + dr) % rows + rows) % rows;
          int nc = ((c + dc) % cols + cols) % cols;
          neighborSum += stateField[nr * Grid::kMaxCols + nc] * kernel[kidx];
          ++kidx;
        }
      }

      // Normalize by kernel sum
      float potential = (kernelSum > 0.0f) ? neighborSum / kernelSum : 0.0f;

      // Growth function: Gaussian centered at mu, width sigma
      float diff = potential - kMu;
      float growth =
          2.0f * std::exp(-0.5f * diff * diff / (kSigma * kSigma)) - 1.0f;

      // Update state
      int idx = r * Grid::kMaxCols + c;
      scratch[idx] = stateField[idx] + kDt * growth;

      // Clamp to [0, 1]
      if (scratch[idx] < 0.0f)
        scratch[idx] = 0.0f;
      if (scratch[idx] > 1.0f)
        scratch[idx] = 1.0f;
    }
  }

  std::copy(scratch.begin(), scratch.end(), stateField.begin());
  projectToGrid();
  ++generation;
}

void LeniaEngine::projectToGrid() {
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      int idx = r * Grid::kMaxCols + c;
      float s = stateField[idx];
      if (s > kThreshold) {
        grid.setCell(r, c, 1);
        grid.setAge(r, c, static_cast<uint16_t>(s * 255.0f));
      } else {
        grid.setCell(r, c, 0);
        grid.setAge(r, c, 0);
      }
    }
  }
}

void LeniaEngine::randomize(uint64_t seed, float density) {
  generation = 0;
  uint64_t state = seed ? seed : 1;

  std::fill(stateField.begin(), stateField.end(), 0.0f);

  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      float val = static_cast<float>(xorshift64(state) >> 32) / 4294967296.0f;
      if (val < density) {
        stateField[r * Grid::kMaxCols + c] = 0.5f + 0.5f * val;
      }
    }
  }

  projectToGrid();
}

void LeniaEngine::randomizeSymmetric(uint64_t seed, float density) {
  generation = 0;
  uint64_t state = seed ? seed : 1;
  std::fill(stateField.begin(), stateField.end(), 0.0f);

  const int halfR = (rows + 1) / 2;
  const int halfC = (cols + 1) / 2;

  for (int r = 0; r < halfR; ++r) {
    for (int c = 0; c < halfC; ++c) {
      float val = static_cast<float>(xorshift64(state) >> 32) / 4294967296.0f;
      if (val < density) {
        float v = 0.5f + 0.5f * val;
        int mr = rows - 1 - r;
        int mc = cols - 1 - c;
        stateField[r * Grid::kMaxCols + c] = v;
        stateField[r * Grid::kMaxCols + mc] = v;
        stateField[mr * Grid::kMaxCols + c] = v;
        stateField[mr * Grid::kMaxCols + mc] = v;
      }
    }
  }

  projectToGrid();
}

void LeniaEngine::clear() {
  generation = 0;
  std::fill(stateField.begin(), stateField.end(), 0.0f);
  grid.clear();
}
