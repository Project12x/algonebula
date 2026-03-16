#pragma once
// Lenia3DCompute -- wraps ghostsun::SeparableLeniaEngine as ComputeSimulation.
// Provides 3D Lenia compute + readback + 2D MIP projection for audio grid.

#include <cstdint>
#include <ghostsun_render/ComputeSimulation.h>
#include <ghostsun_render/SeparableLeniaEngine.h>
#include <memory>

namespace algonebula {

class Lenia3DCompute : public ghostsun::ComputeSimulation {
public:
  Lenia3DCompute(int gridSize = 64);
  ~Lenia3DCompute() override;

  // ComputeSimulation interface
  bool init(WGPUDevice device) override;
  void step(WGPUCommandEncoder encoder) override;
  void shutdown() override;
  std::vector<ghostsun::ReadbackRequest> getReadbackRequests() override;

  // 3D-specific accessors
  int getGridSize() const { return N_; }
  WGPUBuffer getStateBuffer() const;

  // Seeding
  void seed(WGPUQueue queue, uint64_t rngSeed, float density = 0.15f);

private:
  int N_;
  uint32_t cellCount_;
  ghostsun::SeparableLeniaDesc desc_;
  std::unique_ptr<ghostsun::SeparableLeniaEngine> engine_;
  WGPUDevice device_ = nullptr;
};

} // namespace algonebula
