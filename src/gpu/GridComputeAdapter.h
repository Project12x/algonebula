#pragma once
// GridComputeAdapter -- Base class for 2D grid CA compute adapters.
// Handles common boilerplate: ping-pong state buffers, uniform params,
// shader compilation, dispatch, and readback. Subclasses override
// getShaderSource() and getParamsSize() to customize.

#include <cstdint>
#include <ghostsun_render/ComputeSimulation.h>
#include <string>
#include <vector>

namespace algonebula {

class GridComputeAdapter : public ghostsun::ComputeSimulation {
public:
  GridComputeAdapter(int rows, int cols) : rows_(rows), cols_(cols) {}
  ~GridComputeAdapter() override { shutdown(); }

  // --- ComputeSimulation interface ---
  bool init(WGPUDevice device) override;
  void shutdown() override;
  void step(WGPUCommandEncoder encoder) override;
  std::vector<ghostsun::ReadbackRequest> getReadbackRequests() override;
  void seed() override;

  // --- Subclass customization ---

  /// Return the WGSL shader source as a string.
  virtual std::string getShaderSource() const = 0;

  /// Size of the uniform params buffer in bytes (must be 16-byte aligned).
  virtual uint32_t getParamsSize() const = 0;

  /// Write uniform params data. Called before each step.
  virtual void writeParams(void *dst) const = 0;

  /// Entry point name in the WGSL shader. Default: "main".
  virtual const char *getEntryPoint() const { return "main"; }

  /// Number of float elements per cell. Default 1.
  /// RD uses 2 (interleaved u, v).
  virtual uint32_t floatsPerCell() const { return 1; }

  /// Number of extra storage buffers (beyond stateIn/stateOut).
  /// ParticleSwarm and BrownianField use 1 extra (particles/walkers).
  virtual uint32_t extraBufferCount() const { return 0; }

  /// Size in bytes of each extra buffer. Default 0.
  virtual uint64_t extraBufferSize(uint32_t /*idx*/) const { return 0; }

  /// Called after init to allow subclass to fill extra buffer data.
  virtual void initExtraBuffers(WGPUDevice /*device*/, WGPUQueue /*queue*/) {}

  /// Generate initial state data. Default: all zeros.
  virtual void generateInitialState(std::vector<float> &state) const;

  // --- Accessors ---
  int getRows() const { return rows_; }
  int getCols() const { return cols_; }
  uint32_t cellCount() const { return rows_ * cols_; }
  uint32_t stateBufferSize() const {
    return cellCount() * floatsPerCell() * sizeof(float);
  }
  WGPUBuffer getStateBuffer() const { return stateBuffers_[current_]; }
  uint32_t getReadbackFrameSkip() const { return readbackFrameSkip_; }
  void setReadbackFrameSkip(uint32_t n) { readbackFrameSkip_ = n; }

protected:
  int rows_ = 0;
  int cols_ = 0;

  // GPU resources
  WGPUDevice device_ = nullptr;
  WGPUComputePipeline pipeline_ = nullptr;
  WGPUBindGroupLayout bgl_ = nullptr;
  WGPUBuffer stateBuffers_[2] = {}; // ping-pong
  WGPUBuffer paramsBuffer_ = nullptr;
  std::vector<WGPUBuffer> extraBuffers_;
  WGPUBindGroup bindGroups_[2] = {}; // one per ping-pong direction
  int current_ = 0;                  // which stateBuffer is "current"
  uint32_t frame_ = 0;
  uint32_t readbackFrameSkip_ = 2; // readback every N frames
  bool initialized_ = false;

  // Helpers
  WGPUBuffer createBuffer(WGPUDevice device, uint64_t size,
                          WGPUBufferUsage usage, const void *data = nullptr);
  void uploadBuffer(WGPUQueue queue, WGPUBuffer buffer, const void *data,
                    uint64_t size);
};

} // namespace algonebula
