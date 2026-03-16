#include "Lenia3DCompute.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <random>
#include <vector>

namespace algonebula {

Lenia3DCompute::Lenia3DCompute(int gridSize) : N_(gridSize) {
  cellCount_ = static_cast<uint32_t>(N_ * N_ * N_);
  desc_.gridSize = static_cast<uint32_t>(N_);
  desc_.kernelRadius = 5;
  desc_.growthMu = 0.28f;
  desc_.growthSigma = 0.08f;
  desc_.dt = 0.05f;
}

Lenia3DCompute::~Lenia3DCompute() { shutdown(); }

bool Lenia3DCompute::init(WGPUDevice device) {
  device_ = device;
  engine_ = std::make_unique<ghostsun::SeparableLeniaEngine>();
  if (!engine_->init(device, desc_)) {
    fprintf(stderr, "[Lenia3DCompute] SeparableLeniaEngine init failed\n");
    engine_.reset();
    return false;
  }

  // Seed with default blob
  auto queue = wgpuDeviceGetQueue(device);
  seed(queue, 42, 0.15f);

  fprintf(stderr, "[Lenia3DCompute] Initialized %d^3 = %u cells\n", N_,
          cellCount_);
  return true;
}

void Lenia3DCompute::step(WGPUCommandEncoder encoder) {
  if (engine_)
    engine_->step(encoder);
}

void Lenia3DCompute::shutdown() {
  if (engine_) {
    engine_->shutdown();
    engine_.reset();
  }
  device_ = nullptr;
}

std::vector<ghostsun::ReadbackRequest>
Lenia3DCompute::getReadbackRequests() {
  if (!engine_ || !engine_->getStateBuffer())
    return {};

  ghostsun::ReadbackRequest req;
  req.srcBuffer = engine_->getStateBuffer();
  req.offset = 0;
  req.size = static_cast<uint64_t>(cellCount_) * sizeof(float);
  return {req};
}

WGPUBuffer Lenia3DCompute::getStateBuffer() const {
  return engine_ ? engine_->getStateBuffer() : nullptr;
}

void Lenia3DCompute::seed(WGPUQueue queue, uint64_t rngSeed, float density) {
  if (!engine_)
    return;

  std::vector<float> data(cellCount_, 0.0f);
  std::mt19937 rng(static_cast<unsigned>(rngSeed));
  std::uniform_real_distribution<float> dist(0.0f, 1.0f);

  // Create a sphere blob in the center
  float cx = N_ * 0.5f, cy = N_ * 0.5f, cz = N_ * 0.5f;
  float radius = N_ * 0.25f;

  for (int z = 0; z < N_; ++z) {
    for (int y = 0; y < N_; ++y) {
      for (int x = 0; x < N_; ++x) {
        float dx = x - cx, dy = y - cy, dz = z - cz;
        float d = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (d < radius && dist(rng) < density) {
          data[z * N_ * N_ + y * N_ + x] = dist(rng) * 0.8f + 0.2f;
        }
      }
    }
  }

  engine_->uploadState(queue, data.data());
}

} // namespace algonebula
