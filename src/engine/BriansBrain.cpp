#include "BriansBrain.h"

namespace {
uint64_t xorshift64(uint64_t &state) {
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}
} // namespace

BriansBrain::BriansBrain(int rows, int cols)
    : grid(rows, cols), scratch(rows, cols) {}

void BriansBrain::step() {
  const int rows = grid.getRows();
  const int cols = grid.getCols();
  scratch.resize(rows, cols);

  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      uint8_t current = grid.getCell(r, c);

      if (current == 0) {
        // Dead: birth if exactly 2 On neighbors
        int onCount = 0;
        for (int dr = -1; dr <= 1; ++dr) {
          for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0)
              continue;
            if (grid.getCell(r + dr, c + dc) == 1)
              ++onCount;
          }
        }
        if (onCount == 2) {
          scratch.setCell(r, c, 1);
          scratch.setAge(r, c, 1);
        } else {
          scratch.setCell(r, c, 0);
          scratch.setAge(r, c, 0);
        }
      } else if (current == 1) {
        // On -> Dying
        scratch.setCell(r, c, 2);
        scratch.setAge(r, c, grid.getAge(r, c) + 1);
      } else {
        // Dying -> Off
        scratch.setCell(r, c, 0);
        scratch.setAge(r, c, 0);
      }
    }
  }

  grid.copyFrom(scratch);
  ++generation;
}

void BriansBrain::randomize(uint64_t seed, float density) {
  grid.clear();
  generation = 0;
  uint64_t state = seed ? seed : 1;

  for (int r = 0; r < grid.getRows(); ++r) {
    for (int c = 0; c < grid.getCols(); ++c) {
      float val = static_cast<float>(xorshift64(state) >> 32) / 4294967296.0f;
      if (val < density) {
        grid.setCell(r, c, 1); // Born as On
        grid.setAge(r, c, 1);
      }
    }
  }
}

void BriansBrain::randomizeSymmetric(uint64_t seed, float density) {
  grid.clear();
  generation = 0;
  uint64_t state = seed ? seed : 1;
  const int halfR = (grid.getRows() + 1) / 2;
  const int halfC = (grid.getCols() + 1) / 2;

  for (int r = 0; r < halfR; ++r) {
    for (int c = 0; c < halfC; ++c) {
      float val = static_cast<float>(xorshift64(state) >> 32) / 4294967296.0f;
      if (val < density) {
        int mr = grid.getRows() - 1 - r;
        int mc = grid.getCols() - 1 - c;
        grid.setCell(r, c, 1);
        grid.setAge(r, c, 1);
        grid.setCell(r, mc, 1);
        grid.setAge(r, mc, 1);
        grid.setCell(mr, c, 1);
        grid.setAge(mr, c, 1);
        grid.setCell(mr, mc, 1);
        grid.setAge(mr, mc, 1);
      }
    }
  }
}

void BriansBrain::clear() {
  grid.clear();
  generation = 0;
}
