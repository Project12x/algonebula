#pragma once
// GpuGridBridge -- Lock-free bridge between GPU readback (UI thread)
// and audio thread grid snapshot. Double-buffered with atomic swap.
//
// GPU readback delivers float[] state -> UI thread calls updateFromGpu()
// -> audio thread reads getAudioGrid() each processBlock.

#include "engine/Grid.h"
#include <atomic>
#include <cstdint>
#include <vector>

class GpuGridBridge {
public:
  GpuGridBridge() = default;

  /// Configure dimensions. Call before first use.
  void resize(int rows, int cols) {
    rows_ = rows;
    cols_ = cols;
  }

  /// Called on UI thread when GPU readback completes.
  /// Converts float state data to Grid format and swaps the buffer.
  /// @param data  Float array of size rows*cols (row-major, no padding).
  /// @param threshold  Cell activation threshold (values >= threshold ->
  /// alive).
  void updateFromGpu(const float *data, int rows, int cols,
                     float threshold = 0.1f) {
    if (!data || rows <= 0 || cols <= 0)
      return;

    rows_ = rows;
    cols_ = cols;

    // Write to the back grid (not currently being read by audio)
    Grid *writeGrid = writeGrid_.load(std::memory_order_relaxed);

    // Snapshot previous state for birth detection
    writeGrid->resize(rows, cols);

    // Copy previous generation for wasBorn() detection
    if (generation_ > 0) {
      const Grid *readGrid = readGrid_.load(std::memory_order_acquire);
      for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
          writeGrid->setCell(r, c, readGrid->getCell(r, c));
        }
      }
      writeGrid->snapshotPrev();
    }

    // Write new state from GPU float data
    for (int r = 0; r < rows; ++r) {
      for (int c = 0; c < cols; ++c) {
        float val = data[r * cols + c];
        uint8_t cellVal = (val >= threshold) ? 1 : 0;
        writeGrid->setCell(r, c, cellVal);

        // Age: increment if alive, reset if dead
        if (cellVal > 0) {
          writeGrid->incrementAge(r, c);
        } else {
          writeGrid->setAge(r, c, 0);
        }
      }
    }

    ++generation_;

    // Atomic swap: make write grid available to audio thread
    Grid *old = readGrid_.exchange(writeGrid, std::memory_order_release);
    writeGrid_.store(old, std::memory_order_relaxed);

    // Store float data for continuous intensity reads
    {
      if (floatStateBack_.size() < static_cast<size_t>(rows * cols))
        floatStateBack_.resize(rows * cols);
      std::memcpy(floatStateBack_.data(), data, rows * cols * sizeof(float));
      // Swap float buffers
      auto *old = floatRead_.exchange(floatStateBack_.data(),
                                      std::memory_order_release);
      // old points to floatStateFront_ or floatStateBack_ data
      (void)old;
    }
  }

  /// Called on audio thread. Returns the most recent grid snapshot.
  /// Lock-free: just reads the atomic pointer.
  const Grid &getAudioGrid() const {
    return *readGrid_.load(std::memory_order_acquire);
  }

  /// Get continuous float intensity for a cell (audio thread).
  /// Returns 0.0 if no GPU data yet.
  float getCellIntensity(int row, int col) const {
    const float *data = floatRead_.load(std::memory_order_acquire);
    if (!data || row < 0 || row >= rows_ || col < 0 || col >= cols_)
      return 0.0f;
    return data[row * cols_ + col];
  }

  /// Current generation count.
  uint64_t getGeneration() const { return generation_; }

  /// Whether GPU has delivered at least one frame.
  bool hasData() const { return generation_ > 0; }

  int getRows() const { return rows_; }
  int getCols() const { return cols_; }

private:
  Grid gridA_;
  Grid gridB_;
  mutable std::atomic<Grid *> readGrid_{&gridA_};
  std::atomic<Grid *> writeGrid_{&gridB_};

  // Float state double-buffer for continuous intensity
  std::vector<float> floatStateFront_;
  std::vector<float> floatStateBack_;
  mutable std::atomic<float *> floatRead_{nullptr};

  int rows_ = 0;
  int cols_ = 0;
  uint64_t generation_ = 0;
};
