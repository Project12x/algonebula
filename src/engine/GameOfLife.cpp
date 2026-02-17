#include "GameOfLife.h"

// --- Simple xorshift64 PRNG (allocation-free, deterministic) ---
namespace {
uint64_t xorshift64(uint64_t &state) {
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}
} // namespace

// --- Rule bitmask encoding ---
// Birth/Survival rules stored as bitmasks where bit N = "N neighbors triggers"
// e.g. B3/S23 -> birthRule = (1<<3), survivalRule = (1<<2)|(1<<3)
static constexpr uint16_t makeBitmask(std::initializer_list<int> counts) {
  uint16_t mask = 0;
  for (int c : counts)
    mask |= static_cast<uint16_t>(1 << c);
  return mask;
}

GameOfLife::GameOfLife(int rows, int cols, RulePreset preset)
    : grid(rows, cols), scratch(rows, cols) {
  applyPreset(preset);
}

void GameOfLife::applyPreset(RulePreset preset) {
  currentPreset = preset;
  switch (preset) {
  case RulePreset::Classic: // B3/S23
    birthRule = makeBitmask({3});
    survivalRule = makeBitmask({2, 3});
    break;
  case RulePreset::HighLife: // B36/S23
    birthRule = makeBitmask({3, 6});
    survivalRule = makeBitmask({2, 3});
    break;
  case RulePreset::DayAndNight: // B3678/S34678
    birthRule = makeBitmask({3, 6, 7, 8});
    survivalRule = makeBitmask({3, 4, 6, 7, 8});
    break;
  case RulePreset::Seeds: // B2/S (nothing survives)
    birthRule = makeBitmask({2});
    survivalRule = 0;
    break;
  case RulePreset::Ambient: // B3/S2345 (high survival, slow decay)
    birthRule = makeBitmask({3});
    survivalRule = makeBitmask({2, 3, 4, 5});
    break;
  default:
    birthRule = makeBitmask({3});
    survivalRule = makeBitmask({2, 3});
    break;
  }
}

void GameOfLife::setRulePreset(RulePreset preset) { applyPreset(preset); }

void GameOfLife::step() {
  const int rows = grid.getRows();
  const int cols = grid.getCols();

  scratch.resize(rows, cols);

  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      int neighbors = countNeighbors(r, c);
      uint8_t current = grid.getCell(r, c);

      if (current == 0) {
        // Dead cell: check birth rule
        if (birthRule & (1 << neighbors)) {
          scratch.setCell(r, c, 1);
          scratch.setAge(r, c, 1);
        } else {
          scratch.setCell(r, c, 0);
          scratch.setAge(r, c, 0);
        }
      } else {
        // Alive cell: check survival rule
        if (survivalRule & (1 << neighbors)) {
          scratch.setCell(r, c, 1);
          // Increment age (carry from current grid)
          uint16_t age = grid.getAge(r, c);
          scratch.setAge(r, c, age < UINT16_MAX ? age + 1 : age);
        } else {
          scratch.setCell(r, c, 0);
          scratch.setAge(r, c, 0);
        }
      }
    }
  }

  // Swap scratch into grid (memcpy, no allocation)
  grid.copyFrom(scratch);
  ++generation;
}

void GameOfLife::randomize(uint64_t seed, float density) {
  grid.clear();
  generation = 0;

  uint64_t state = seed;
  if (state == 0)
    state = 1; // xorshift64 needs non-zero seed

  const int rows = grid.getRows();
  const int cols = grid.getCols();

  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      uint64_t rng = xorshift64(state);
      // Use upper bits for better distribution
      float normalized = static_cast<float>(rng >> 32) / 4294967296.0f;
      if (normalized < density) {
        grid.setCell(r, c, 1);
        grid.setAge(r, c, 1);
      }
    }
  }
}

void GameOfLife::randomizeSymmetric(uint64_t seed, float density) {
  grid.clear();
  generation = 0;

  uint64_t state = seed;
  if (state == 0)
    state = 1;

  const int rows = grid.getRows();
  const int cols = grid.getCols();
  const int halfR = (rows + 1) / 2; // include center row
  const int halfC = (cols + 1) / 2; // include center col

  // Generate random cells in top-left quadrant, mirror to all four
  for (int r = 0; r < halfR; ++r) {
    for (int c = 0; c < halfC; ++c) {
      uint64_t rng = xorshift64(state);
      float normalized = static_cast<float>(rng >> 32) / 4294967296.0f;
      if (normalized < density) {
        int mirrorR = rows - 1 - r;
        int mirrorC = cols - 1 - c;

        grid.setCell(r, c, 1);
        grid.setAge(r, c, 1);
        grid.setCell(r, mirrorC, 1);
        grid.setAge(r, mirrorC, 1);
        grid.setCell(mirrorR, c, 1);
        grid.setAge(mirrorR, c, 1);
        grid.setCell(mirrorR, mirrorC, 1);
        grid.setAge(mirrorR, mirrorC, 1);
      }
    }
  }
}

void GameOfLife::clear() {
  grid.clear();
  generation = 0;
}

void GameOfLife::loadPattern(const int (*cells)[2], int count, int originRow,
                             int originCol) {
  grid.clear();
  generation = 0;

  for (int i = 0; i < count; ++i) {
    int r = originRow + cells[i][0];
    int c = originCol + cells[i][1];
    grid.setCell(r, c, 1);
    grid.setAge(r, c, 1);
  }
}

int GameOfLife::countNeighbors(int row, int col) const {
  int count = 0;
  for (int dr = -1; dr <= 1; ++dr) {
    for (int dc = -1; dc <= 1; ++dc) {
      if (dr == 0 && dc == 0)
        continue;
      if (grid.getCell(row + dr, col + dc) > 0)
        ++count;
    }
  }
  return count;
}
