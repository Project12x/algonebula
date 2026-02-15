#pragma once

#include <atomic>
#include <cstdint>

/// Lock-free SPSC (Single Producer, Single Consumer) queue for cell edit
/// commands. UI thread pushes edits, audio thread drains at start of
/// processBlock.
///
/// Fixed capacity, no allocations after construction.
class CellEditQueue {
public:
  struct Command {
    int row;
    int col;
    uint8_t state; // 0 = dead, 1 = alive
  };

  static constexpr int kCapacity = 256;

  /// Push a command (UI thread). Returns false if queue is full.
  bool push(int row, int col, uint8_t state) {
    const int w = writePos.load(std::memory_order_relaxed);
    const int nextW = (w + 1) % kCapacity;

    if (nextW == readPos.load(std::memory_order_acquire))
      return false; // Full

    buffer[w] = {row, col, state};
    writePos.store(nextW, std::memory_order_release);
    return true;
  }

  /// Pop a command (audio thread). Returns false if queue is empty.
  bool pop(Command &cmd) {
    const int r = readPos.load(std::memory_order_relaxed);

    if (r == writePos.load(std::memory_order_acquire))
      return false; // Empty

    cmd = buffer[r];
    readPos.store((r + 1) % kCapacity, std::memory_order_release);
    return true;
  }

  /// Drain up to maxCount commands into grid. Returns number drained.
  /// Call from audio thread at start of processBlock.
  template <typename GridType>
  int drainInto(GridType &grid, int maxCount = 64) {
    int count = 0;
    Command cmd;
    while (count < maxCount && pop(cmd)) {
      grid.setCell(cmd.row, cmd.col, cmd.state);
      if (cmd.state > 0)
        grid.setAge(cmd.row, cmd.col, 1);
      else
        grid.setAge(cmd.row, cmd.col, 0);
      ++count;
    }
    return count;
  }

private:
  Command buffer[kCapacity] = {};
  alignas(64) std::atomic<int> readPos{0};
  alignas(64) std::atomic<int> writePos{0};
};
