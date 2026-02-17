#include "ReactionDiffusion.h"
#include <cmath>

namespace {
uint64_t xorshift64(uint64_t &state) {
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}
} // namespace

ReactionDiffusion::ReactionDiffusion(int r, int c)
    : grid(r, c), rows(r), cols(c) {
  // Initialize field A to 1.0 everywhere
  for (int i = 0; i < rows * cols; ++i) {
    fieldA[i] = 1.0f;
    fieldB[i] = 0.0f;
  }
}

void ReactionDiffusion::step() {
  // 5-point Laplacian stencil with toroidal wrapping
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      int idx = r * Grid::kMaxCols + c;

      // Toroidal neighbors
      int up = ((r - 1 + rows) % rows) * Grid::kMaxCols + c;
      int dn = ((r + 1) % rows) * Grid::kMaxCols + c;
      int lt = r * Grid::kMaxCols + ((c - 1 + cols) % cols);
      int rt = r * Grid::kMaxCols + ((c + 1) % cols);

      // Laplacian
      float lapA = fieldA[up] + fieldA[dn] + fieldA[lt] + fieldA[rt] -
                   4.0f * fieldA[idx];
      float lapB = fieldB[up] + fieldB[dn] + fieldB[lt] + fieldB[rt] -
                   4.0f * fieldB[idx];

      float a = fieldA[idx];
      float b = fieldB[idx];
      float ab2 = a * b * b;

      // Gray-Scott equations
      scratchA[idx] = a + kDt * (kDa * lapA - ab2 + kFeed * (1.0f - a));
      scratchB[idx] = b + kDt * (kDb * lapB + ab2 - (kFeed + kKill) * b);

      // Clamp to [0, 1]
      if (scratchA[idx] < 0.0f)
        scratchA[idx] = 0.0f;
      if (scratchA[idx] > 1.0f)
        scratchA[idx] = 1.0f;
      if (scratchB[idx] < 0.0f)
        scratchB[idx] = 0.0f;
      if (scratchB[idx] > 1.0f)
        scratchB[idx] = 1.0f;
    }
  }

  // Swap
  std::memcpy(fieldA, scratchA, sizeof(float) * Grid::kMaxCells);
  std::memcpy(fieldB, scratchB, sizeof(float) * Grid::kMaxCells);

  projectToGrid();
  ++generation;
}

void ReactionDiffusion::projectToGrid() {
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      int idx = r * Grid::kMaxCols + c;
      float b = fieldB[idx];
      if (b > kThreshold) {
        grid.setCell(r, c, 1);
        grid.setAge(r, c, static_cast<uint16_t>(b * 255.0f));
      } else {
        grid.setCell(r, c, 0);
        grid.setAge(r, c, 0);
      }
    }
  }
}

void ReactionDiffusion::randomize(uint64_t seed, float density) {
  generation = 0;
  uint64_t state = seed ? seed : 1;

  // Fill A=1 everywhere, seed B in random spots
  for (int i = 0; i < Grid::kMaxCells; ++i) {
    fieldA[i] = 1.0f;
    fieldB[i] = 0.0f;
  }

  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      float val = static_cast<float>(xorshift64(state) >> 32) / 4294967296.0f;
      if (val < density) {
        int idx = r * Grid::kMaxCols + c;
        fieldB[idx] = 1.0f;
        fieldA[idx] = 0.0f;
      }
    }
  }

  projectToGrid();
}

void ReactionDiffusion::randomizeSymmetric(uint64_t seed, float density) {
  generation = 0;
  uint64_t state = seed ? seed : 1;

  for (int i = 0; i < Grid::kMaxCells; ++i) {
    fieldA[i] = 1.0f;
    fieldB[i] = 0.0f;
  }

  const int halfR = (rows + 1) / 2;
  const int halfC = (cols + 1) / 2;

  for (int r = 0; r < halfR; ++r) {
    for (int c = 0; c < halfC; ++c) {
      float val = static_cast<float>(xorshift64(state) >> 32) / 4294967296.0f;
      if (val < density) {
        int mr = rows - 1 - r;
        int mc = cols - 1 - c;
        auto seed_cell = [&](int rr, int cc) {
          int idx = rr * Grid::kMaxCols + cc;
          fieldB[idx] = 1.0f;
          fieldA[idx] = 0.0f;
        };
        seed_cell(r, c);
        seed_cell(r, mc);
        seed_cell(mr, c);
        seed_cell(mr, mc);
      }
    }
  }

  projectToGrid();
}

void ReactionDiffusion::clear() {
  generation = 0;
  for (int i = 0; i < Grid::kMaxCells; ++i) {
    fieldA[i] = 1.0f;
    fieldB[i] = 0.0f;
  }
  grid.clear();
}
