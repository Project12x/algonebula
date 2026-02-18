#include "ParticleSwarm.h"
#include <algorithm>
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

ParticleSwarm::ParticleSwarm(int r, int c)
    : trail(Grid::kMaxCells, 0.0f), grid(r, c), rows(r), cols(c) {}

void ParticleSwarm::step() {
  // Compute center of mass
  float cx = 0.0f, cy = 0.0f;
  for (int i = 0; i < kNumParticles; ++i) {
    cx += particles[i].x;
    cy += particles[i].y;
  }
  cx /= kNumParticles;
  cy /= kNumParticles;

  // Update each particle
  for (int i = 0; i < kNumParticles; ++i) {
    auto &p = particles[i];

    // Flocking: average velocity of nearby particles
    float avgVx = 0.0f, avgVy = 0.0f;
    int neighbors = 0;
    for (int j = 0; j < kNumParticles; ++j) {
      if (j == i)
        continue;
      float dx = particles[j].x - p.x;
      float dy = particles[j].y - p.y;
      float dist = std::sqrt(dx * dx + dy * dy);
      if (dist < 4.0f) {
        avgVx += particles[j].vx;
        avgVy += particles[j].vy;
        ++neighbors;
      }
    }
    if (neighbors > 0) {
      avgVx /= neighbors;
      avgVy /= neighbors;
      p.vx += (avgVx - p.vx) * kFlockWeight;
      p.vy += (avgVy - p.vy) * kFlockWeight;
    }

    // Attract toward center of mass
    p.vx += (cx - p.x) * kCenterWeight;
    p.vy += (cy - p.y) * kCenterWeight;

    // Random jitter
    p.vx += (randFloat(rng) - 0.5f) * 0.3f;
    p.vy += (randFloat(rng) - 0.5f) * 0.3f;

    // Clamp speed
    float speed = std::sqrt(p.vx * p.vx + p.vy * p.vy);
    if (speed > kMaxSpeed) {
      p.vx = p.vx / speed * kMaxSpeed;
      p.vy = p.vy / speed * kMaxSpeed;
    }

    // Move (toroidal)
    p.x += p.vx;
    p.y += p.vy;
    if (p.x < 0)
      p.x += cols;
    if (p.x >= cols)
      p.x -= cols;
    if (p.y < 0)
      p.y += rows;
    if (p.y >= rows)
      p.y -= rows;

    // Deposit trail
    int gr = static_cast<int>(p.y) % rows;
    int gc = static_cast<int>(p.x) % cols;
    if (gr >= 0 && gr < rows && gc >= 0 && gc < cols) {
      trail[gr * Grid::kMaxCols + gc] = 1.0f;
    }
  }

  // Decay trail
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      float &t = trail[r * Grid::kMaxCols + c];
      t *= kTrailDecay;
      if (t < 0.01f)
        t = 0.0f;
    }
  }

  projectToGrid();
  ++generation;
}

void ParticleSwarm::projectToGrid() {
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      float t = trail[r * Grid::kMaxCols + c];
      if (t > 0.05f) {
        grid.setCell(r, c, 1);
        grid.setAge(r, c, static_cast<uint16_t>(t * 255.0f));
      } else {
        grid.setCell(r, c, 0);
        grid.setAge(r, c, 0);
      }
    }
  }
}

void ParticleSwarm::randomize(uint64_t seed, float density) {
  generation = 0;
  rng = seed ? seed : 1;
  (void)density;

  std::fill(trail.begin(), trail.end(), 0.0f);

  for (int i = 0; i < kNumParticles; ++i) {
    particles[i].x = randFloat(rng) * cols;
    particles[i].y = randFloat(rng) * rows;
    particles[i].vx = (randFloat(rng) - 0.5f) * 2.0f;
    particles[i].vy = (randFloat(rng) - 0.5f) * 2.0f;
  }

  projectToGrid();
}

void ParticleSwarm::randomizeSymmetric(uint64_t seed, float density) {
  generation = 0;
  rng = seed ? seed : 1;
  (void)density;

  std::fill(trail.begin(), trail.end(), 0.0f);

  // Place particles symmetrically (4 quadrants)
  int perQuadrant = kNumParticles / 4;
  for (int i = 0; i < perQuadrant; ++i) {
    float x = randFloat(rng) * (cols / 2.0f);
    float y = randFloat(rng) * (rows / 2.0f);
    float vx = (randFloat(rng) - 0.5f) * 2.0f;
    float vy = (randFloat(rng) - 0.5f) * 2.0f;

    int base = i * 4;
    particles[base] = {x, y, vx, vy};
    particles[base + 1] = {cols - x, y, -vx, vy};
    particles[base + 2] = {x, rows - y, vx, -vy};
    particles[base + 3] = {cols - x, rows - y, -vx, -vy};
  }

  projectToGrid();
}

void ParticleSwarm::clear() {
  generation = 0;
  std::fill(trail.begin(), trail.end(), 0.0f);
  for (int i = 0; i < kNumParticles; ++i)
    particles[i] = {};
  grid.clear();
}
