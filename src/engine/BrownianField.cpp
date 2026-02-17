#include "BrownianField.h"
#include <cmath>

namespace {
uint64_t xorshift64(uint64_t &state) {
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}

float randFloat(uint64_t &state) {
  return static_cast<float>(xorshift64(state) >> 32) / 4294967296.0f;
}
} // namespace

BrownianField::BrownianField(int r, int c) : grid(r, c), rows(r), cols(c) {}

void BrownianField::step() {
  // Move walkers (random walk)
  for (int i = 0; i < kNumWalkers; ++i) {
    auto &w = walkers[i];

    // Random step in x and y
    w.x += (randFloat(rng) - 0.5f) * 2.0f;
    w.y += (randFloat(rng) - 0.5f) * 2.0f;

    // Toroidal wrapping
    if (w.x < 0)
      w.x += cols;
    if (w.x >= cols)
      w.x -= cols;
    if (w.y < 0)
      w.y += rows;
    if (w.y >= rows)
      w.y -= rows;

    // Deposit energy at current position
    int gr = static_cast<int>(w.y) % rows;
    int gc = static_cast<int>(w.x) % cols;
    if (gr >= 0 && gr < rows && gc >= 0 && gc < cols) {
      float &e = energy[gr * Grid::kMaxCols + gc];
      e += kDepositAmount;
      if (e > 1.0f)
        e = 1.0f;
    }
  }

  // Global energy decay
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      float &e = energy[r * Grid::kMaxCols + c];
      e *= kEnergyDecay;
      if (e < 0.01f)
        e = 0.0f;
    }
  }

  projectToGrid();
  ++generation;
}

void BrownianField::projectToGrid() {
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      float e = energy[r * Grid::kMaxCols + c];
      if (e > kThreshold) {
        grid.setCell(r, c, 1);
        grid.setAge(r, c, static_cast<uint16_t>(e * 255.0f));
      } else {
        grid.setCell(r, c, 0);
        grid.setAge(r, c, 0);
      }
    }
  }
}

void BrownianField::randomize(uint64_t seed, float density) {
  generation = 0;
  rng = seed ? seed : 1;
  (void)density;

  std::memset(energy, 0, sizeof(energy));

  for (int i = 0; i < kNumWalkers; ++i) {
    walkers[i].x = randFloat(rng) * cols;
    walkers[i].y = randFloat(rng) * rows;
  }

  projectToGrid();
}

void BrownianField::randomizeSymmetric(uint64_t seed, float density) {
  generation = 0;
  rng = seed ? seed : 1;
  (void)density;

  std::memset(energy, 0, sizeof(energy));

  // Place walkers symmetrically
  int perQuadrant = kNumWalkers / 4;
  for (int i = 0; i < perQuadrant; ++i) {
    float x = randFloat(rng) * (cols / 2.0f);
    float y = randFloat(rng) * (rows / 2.0f);

    int base = i * 4;
    walkers[base] = {x, y};
    walkers[base + 1] = {static_cast<float>(cols) - x, y};
    walkers[base + 2] = {x, static_cast<float>(rows) - y};
    walkers[base + 3] = {static_cast<float>(cols) - x,
                         static_cast<float>(rows) - y};
  }

  projectToGrid();
}

void BrownianField::clear() {
  generation = 0;
  std::memset(energy, 0, sizeof(energy));
  for (int i = 0; i < kNumWalkers; ++i)
    walkers[i] = {};
  grid.clear();
}
