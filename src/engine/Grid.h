#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

/// Grid data structure for cellular automata.
/// Stores cell state (uint8) and cell age (uint16) in row-major order.
/// Supports double-buffering: audio thread owns the working grid,
/// swaps a snapshot for the GL thread to read.
class Grid {
public:
  static constexpr int kMaxRows = 512;
  static constexpr int kMaxCols = 512;
  static constexpr int kMaxCells = kMaxRows * kMaxCols;

  Grid() : cells(kMaxCells, 0), prevCells(kMaxCells, 0), ages(kMaxCells, 0) {}

  Grid(int rows, int cols)
      : numRows(rows), numCols(cols), cells(kMaxCells, 0),
        prevCells(kMaxCells, 0), ages(kMaxCells, 0) {
    clampDimensions();
  }

  // --- Dimensions ---
  int getRows() const { return numRows; }
  int getCols() const { return numCols; }

  void resize(int rows, int cols) {
    numRows = rows;
    numCols = cols;
    clampDimensions();
    clear();
  }

  // --- Cell State Access ---
  uint8_t getCell(int row, int col) const {
    return cells[wrapRow(row) * kMaxCols + wrapCol(col)];
  }

  void setCell(int row, int col, uint8_t value) {
    cells[wrapRow(row) * kMaxCols + wrapCol(col)] = value;
  }

  // --- Cell Age Access ---
  uint16_t getAge(int row, int col) const {
    return ages[wrapRow(row) * kMaxCols + wrapCol(col)];
  }

  void setAge(int row, int col, uint16_t value) {
    ages[wrapRow(row) * kMaxCols + wrapCol(col)] = value;
  }

  void incrementAge(int row, int col) {
    auto &a = ages[wrapRow(row) * kMaxCols + wrapCol(col)];
    if (a < UINT16_MAX)
      ++a;
  }

  // --- Bulk Operations ---
  void clear() {
    std::fill(cells.begin(), cells.end(), 0);
    std::fill(ages.begin(), ages.end(), 0);
  }

  void copyFrom(const Grid &other) {
    numRows = other.numRows;
    numCols = other.numCols;
    cells = other.cells;
    ages = other.ages;
  }

  /// Count total alive cells (state > 0).
  int countAlive() const {
    int count = 0;
    for (int r = 0; r < numRows; ++r)
      for (int c = 0; c < numCols; ++c)
        if (cells[r * kMaxCols + c] > 0)
          ++count;
    return count;
  }

  /// Get density as a fraction [0.0, 1.0].
  float getDensity() const {
    int total = numRows * numCols;
    return (total > 0) ? static_cast<float>(countAlive()) / total : 0.0f;
  }

  /// Check grid equality (dimensions + cell states).
  bool operator==(const Grid &other) const {
    if (numRows != other.numRows || numCols != other.numCols)
      return false;
    for (int r = 0; r < numRows; ++r)
      for (int c = 0; c < numCols; ++c)
        if (cells[r * kMaxCols + c] != other.cells[r * kMaxCols + c])
          return false;
    return true;
  }

  bool operator!=(const Grid &other) const { return !(*this == other); }

  // --- Toroidal Wrapping ---
  int wrapRow(int r) const {
    r %= numRows;
    return r < 0 ? r + numRows : r;
  }

  int wrapCol(int c) const {
    c %= numCols;
    return c < 0 ? c + numCols : c;
  }

  // --- Event Detection (birth/death tracking) ---
  /// Call before engine step to snapshot current state.
  void snapshotPrev() { prevCells = cells; }

  /// Cell was dead last step, alive now.
  bool wasBorn(int row, int col) const {
    int idx = wrapRow(row) * kMaxCols + wrapCol(col);
    return prevCells[idx] == 0 && cells[idx] > 0;
  }

  /// Cell was alive last step, dead now.
  bool justDied(int row, int col) const {
    int idx = wrapRow(row) * kMaxCols + wrapCol(col);
    return prevCells[idx] > 0 && cells[idx] == 0;
  }

  /// Cell was alive last step and still alive.
  bool persists(int row, int col) const {
    int idx = wrapRow(row) * kMaxCols + wrapCol(col);
    return prevCells[idx] > 0 && cells[idx] > 0;
  }

private:
  void clampDimensions() {
    if (numRows < 1)
      numRows = 1;
    if (numRows > kMaxRows)
      numRows = kMaxRows;
    if (numCols < 1)
      numCols = 1;
    if (numCols > kMaxCols)
      numCols = kMaxCols;
  }

  int numRows = 12;
  int numCols = 16;

  // Heap-allocated arrays sized to max capacity (safe for large grids).
  std::vector<uint8_t> cells;
  std::vector<uint8_t> prevCells; // Previous generation for event detection
  std::vector<uint16_t> ages;
};
