#pragma once
// GpuGridBridge -- Lock-free bridge between simulation (GPU or CPU) and
// audio/UI consumers. Float-only hot path: GPU readback is just memcpy +
// atomic swap. Optional Grid conversion for binary engines.
//
// Write side (one writer at a time):
//   GPU readback:  updateFromGpu(float*, rows, cols)   -- message thread
//   CPU engine:    updateFromCpu(Grid&)                 -- audio thread
//
// Read side (lock-free, any thread):
//   getCellIntensity(r, c) -- continuous 0.0-1.0
//   isAlive(r, c)          -- thresholded binary check
//   wasBorn(r, c)          -- birth detection (current alive, previous dead)
//   countAlive()           -- count cells above threshold
//   getDensity()           -- fraction of alive cells

#include "engine/Grid.h"
#include <atomic>
#include <cstdint>
#include <cstring>
#include <vector>

class GpuGridBridge {
public:
  GpuGridBridge() = default;

  /// Configure dimensions. Call before first use.
  void resize(int rows, int cols) {
    rows_ = rows;
    cols_ = cols;
    size_t n = static_cast<size_t>(rows) * cols;
    bufA_.resize(n, 0.0f);
    bufB_.resize(n, 0.0f);
    prevBuf_.resize(n, 0.0f);
    readBuf_.store(bufA_.data(), std::memory_order_relaxed);
    writeBuf_.store(bufB_.data(), std::memory_order_relaxed);
  }

  // ---------------------------------------------------------------
  // Write side
  // ---------------------------------------------------------------

  /// Called on message thread when GPU readback completes.
  /// Hot path: memcpy + atomic swap. No per-cell conversion.
  void updateFromGpu(const float *data, int rows, int cols) {
    if (!data || rows <= 0 || cols <= 0)
      return;

    if (rows != rows_ || cols != cols_)
      resize(rows, cols);

    size_t n = static_cast<size_t>(rows) * cols;
    float *dst = writeBuf_.load(std::memory_order_relaxed);

    // Snapshot current as previous (for birth detection)
    float *cur = readBuf_.load(std::memory_order_acquire);
    std::memcpy(prevBuf_.data(), cur, n * sizeof(float));

    // Copy new data
    std::memcpy(dst, data, n * sizeof(float));

    // Atomic swap: make write buffer the new read buffer
    float *old = readBuf_.exchange(dst, std::memory_order_release);
    writeBuf_.store(old, std::memory_order_relaxed);

    ++generation_;
  }

  /// Called on audio thread after CPU engine steps.
  /// Converts Grid cell values to floats (fast for small grids).
  void updateFromCpu(const Grid &grid) {
    int rows = grid.getRows();
    int cols = grid.getCols();

    if (rows != rows_ || cols != cols_)
      resize(rows, cols);

    size_t n = static_cast<size_t>(rows) * cols;
    float *dst = writeBuf_.load(std::memory_order_relaxed);

    // Snapshot current as previous
    float *cur = readBuf_.load(std::memory_order_acquire);
    std::memcpy(prevBuf_.data(), cur, n * sizeof(float));

    // Convert Grid cells to float
    for (int r = 0; r < rows; ++r) {
      for (int c = 0; c < cols; ++c) {
        dst[r * cols + c] = (grid.getCell(r, c) > 0) ? 1.0f : 0.0f;
      }
    }

    float *old = readBuf_.exchange(dst, std::memory_order_release);
    writeBuf_.store(old, std::memory_order_relaxed);

    ++generation_;
  }

  // ---------------------------------------------------------------
  // Read side (lock-free)
  // ---------------------------------------------------------------

  /// Fetch current and previous float buffers for bulk reading.
  /// Must call readUnlock() when done.
  bool readLock(const float *&current, const float *&previous) const {
    current = readBuf_.load(std::memory_order_acquire);
    previous = prevBuf_.data();
    return current != nullptr;
  }

  void readUnlock() const {
    // No-op for now. Future: could use hazard pointers or a spinlock
    // if writer needs to wait. For double-buffering with memcpy,
    // readBuf_ gives us a stable buffer while writer uses the other.
  }

  /// Continuous float intensity from a locked buffer.
  static float getCellIntensityLocked(const float *current, int rows, int cols,
                                      int row, int col) {
    if (row < 0 || row >= rows || col < 0 || col >= cols)
      return 0.0f;
    return current[row * cols + col];
  }

  /// Whether a cell is above the alive threshold from a locked buffer.
  static bool isAliveLocked(const float *current, int rows, int cols, int row,
                            int col, float threshold = 0.1f) {
    return getCellIntensityLocked(current, rows, cols, row, col) >= threshold;
  }

  /// Whether a cell was just born from locked buffers.
  static bool wasBornLocked(const float *current, const float *previous,
                            int rows, int cols, int row, int col,
                            uint64_t generation, float threshold = 0.1f) {
    if (generation < 2)
      return false;
    if (row < 0 || row >= rows || col < 0 || col >= cols)
      return false;
    int idx = row * cols + col;
    return current[idx] >= threshold && previous[idx] < threshold;
  }

  /// Count of cells above threshold from a locked buffer.
  int countAlive(float threshold = 0.1f) const {
    const float *data = readBuf_.load(std::memory_order_acquire);
    if (!data)
      return 0;
    int count = 0;
    int n = rows_ * cols_;
    for (int i = 0; i < n; ++i) {
      if (data[i] >= threshold)
        ++count;
    }
    return count;
  }

  /// Fraction of alive cells.
  float getDensity(float threshold = 0.1f) const {
    int n = rows_ * cols_;
    if (n == 0)
      return 0.0f;
    return static_cast<float>(countAlive(threshold)) / n;
  }

  /// Current generation count.
  uint64_t getGeneration() const { return generation_; }

  /// Whether at least one frame has been delivered.
  bool hasData() const { return generation_ > 0; }

  int getRows() const { return rows_; }
  int getCols() const { return cols_; }

  // ---------------------------------------------------------------
  // Optional Grid conversion (for binary engines / legacy compat)
  // ---------------------------------------------------------------

  /// Build a Grid snapshot from the current float buffer.
  /// Expensive at large sizes — use sparingly.
  void convertToGrid(Grid &out, float threshold = 0.1f) const {
    out.resize(rows_, cols_);
    const float *data = readBuf_.load(std::memory_order_acquire);
    if (!data)
      return;
    for (int r = 0; r < rows_; ++r) {
      for (int c = 0; c < cols_; ++c) {
        float val = data[r * cols_ + c];
        uint8_t cellState = (val >= threshold) ? 1 : 0;
        out.setCell(r, c, cellState);
        // Store float intensity as age (0-255) for continuous engine rendering
        uint16_t intensity = static_cast<uint16_t>(std::min(val, 1.0f) * 255.0f);
        out.setAge(r, c, intensity);
      }
    }
  }

private:
  // Double-buffered float arrays
  std::vector<float> bufA_;
  std::vector<float> bufB_;
  std::vector<float> prevBuf_; // previous frame for birth detection

  mutable std::atomic<float *> readBuf_{nullptr};
  std::atomic<float *> writeBuf_{nullptr};

  int rows_ = 0;
  int cols_ = 0;
  uint64_t generation_ = 0;
};
