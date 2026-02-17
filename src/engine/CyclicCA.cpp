#include "CyclicCA.h"

namespace {
uint64_t xorshift64(uint64_t &state) {
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}
} // namespace

CyclicCA::CyclicCA(int rows, int cols)
    : grid(rows, cols), scratch(rows, cols) {}

void CyclicCA::step() {
  const int rows = grid.getRows();
  const int cols = grid.getCols();
  scratch.resize(rows, cols);

  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      uint8_t current = grid.getCell(r, c);
      uint8_t next = (current + 1) % kNumStates;

      // Check if any Moore neighbor is at (current+1)%N
      bool consumed = false;
      for (int dr = -1; dr <= 1 && !consumed; ++dr) {
        for (int dc = -1; dc <= 1 && !consumed; ++dc) {
          if (dr == 0 && dc == 0)
            continue;
          if (grid.getCell(r + dr, c + dc) == next)
            consumed = true;
        }
      }

      if (consumed) {
        scratch.setCell(r, c, next);
        scratch.setAge(r, c, 1);
      } else {
        scratch.setCell(r, c, current);
        uint16_t age = grid.getAge(r, c);
        scratch.setAge(r, c, age < UINT16_MAX ? age + 1 : age);
      }
    }
  }

  grid.copyFrom(scratch);
  ++generation;
}

void CyclicCA::randomize(uint64_t seed, float density) {
  grid.clear();
  generation = 0;
  uint64_t state = seed ? seed : 1;
  (void)density; // Cyclic CA always fills all cells with random states

  for (int r = 0; r < grid.getRows(); ++r) {
    for (int c = 0; c < grid.getCols(); ++c) {
      uint8_t s = static_cast<uint8_t>(xorshift64(state) % kNumStates);
      grid.setCell(r, c, s);
      grid.setAge(r, c, 1);
    }
  }
}

void CyclicCA::randomizeSymmetric(uint64_t seed, float density) {
  grid.clear();
  generation = 0;
  uint64_t state = seed ? seed : 1;
  (void)density;
  const int halfR = (grid.getRows() + 1) / 2;
  const int halfC = (grid.getCols() + 1) / 2;

  for (int r = 0; r < halfR; ++r) {
    for (int c = 0; c < halfC; ++c) {
      uint8_t s = static_cast<uint8_t>(xorshift64(state) % kNumStates);
      int mr = grid.getRows() - 1 - r;
      int mc = grid.getCols() - 1 - c;
      grid.setCell(r, c, s);
      grid.setAge(r, c, 1);
      grid.setCell(r, mc, s);
      grid.setAge(r, mc, 1);
      grid.setCell(mr, c, s);
      grid.setAge(mr, c, 1);
      grid.setCell(mr, mc, s);
      grid.setAge(mr, mc, 1);
    }
  }
}

void CyclicCA::clear() {
  grid.clear();
  generation = 0;
}
