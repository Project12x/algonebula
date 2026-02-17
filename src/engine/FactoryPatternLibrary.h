#pragma once

#include "Grid.h"
#include <array>
#include <cstring>

/// Factory pattern library for classic Game of Life patterns.
/// Patterns are stamped centered in the grid.
class FactoryPatternLibrary {
public:
  struct Cell {
    int row, col;
  };

  struct Pattern {
    const char *name;
    const Cell *cells;
    int cellCount;
  };

  // --- Pattern data ---
  static constexpr Cell kGlider[] = {{0, 1}, {1, 2}, {2, 0}, {2, 1}, {2, 2}};

  static constexpr Cell kLWSS[] = {{0, 1}, {0, 4}, {1, 0}, {2, 0}, {2, 4},
                                   {3, 0}, {3, 1}, {3, 2}, {3, 3}};

  static constexpr Cell kRPentomino[] = {
      {0, 1}, {0, 2}, {1, 0}, {1, 1}, {2, 1}};

  static constexpr Cell kPulsar[] = {
      // Top-left quadrant (reflected 4 ways)
      {1, 3},  {1, 4},  {1, 5},   {1, 9},  {1, 10},  {1, 11}, {3, 1},
      {3, 6},  {3, 8},  {3, 13},  {4, 1},  {4, 6},   {4, 8},  {4, 13},
      {5, 1},  {5, 6},  {5, 8},   {5, 13}, {6, 3},   {6, 4},  {6, 5},
      {6, 9},  {6, 10}, {6, 11},  {8, 3},  {8, 4},   {8, 5},  {8, 9},
      {8, 10}, {8, 11}, {9, 1},   {9, 6},  {9, 8},   {9, 13}, {10, 1},
      {10, 6}, {10, 8}, {10, 13}, {11, 1}, {11, 6},  {11, 8}, {11, 13},
      {13, 3}, {13, 4}, {13, 5},  {13, 9}, {13, 10}, {13, 11}};

  static constexpr Cell kGosperGun[] = {
      {1, 25}, {2, 23}, {2, 25}, {3, 13}, {3, 14}, {3, 21}, {3, 22}, {3, 35},
      {3, 36}, {4, 12}, {4, 16}, {4, 21}, {4, 22}, {4, 35}, {4, 36}, {5, 1},
      {5, 2},  {5, 11}, {5, 17}, {5, 21}, {5, 22}, {6, 1},  {6, 2},  {6, 11},
      {6, 15}, {6, 17}, {6, 18}, {6, 23}, {6, 25}, {7, 11}, {7, 17}, {7, 25},
      {8, 12}, {8, 16}, {9, 13}, {9, 14}};

  static constexpr int kPatternCount = 5;

  static const Pattern &getPattern(int index) {
    static const Pattern patterns[kPatternCount] = {
        {"Glider", kGlider, static_cast<int>(std::size(kGlider))},
        {"LWSS", kLWSS, static_cast<int>(std::size(kLWSS))},
        {"R-Pentomino", kRPentomino, static_cast<int>(std::size(kRPentomino))},
        {"Pulsar", kPulsar, static_cast<int>(std::size(kPulsar))},
        {"Gosper Gun", kGosperGun, static_cast<int>(std::size(kGosperGun))}};
    return patterns[index];
  }

  /// Apply a factory pattern to the grid, centered.
  static void applyPattern(Grid &grid, int patternIdx) {
    if (patternIdx < 0 || patternIdx >= kPatternCount)
      return;

    grid.clear();
    const auto &pat = getPattern(patternIdx);

    // Find pattern bounding box for centering
    int maxR = 0, maxC = 0;
    for (int i = 0; i < pat.cellCount; ++i) {
      if (pat.cells[i].row > maxR)
        maxR = pat.cells[i].row;
      if (pat.cells[i].col > maxC)
        maxC = pat.cells[i].col;
    }

    int offsetR = (grid.getRows() - maxR - 1) / 2;
    int offsetC = (grid.getCols() - maxC - 1) / 2;
    if (offsetR < 0)
      offsetR = 0;
    if (offsetC < 0)
      offsetC = 0;

    for (int i = 0; i < pat.cellCount; ++i) {
      int r = pat.cells[i].row + offsetR;
      int c = pat.cells[i].col + offsetC;
      if (r >= 0 && r < grid.getRows() && c >= 0 && c < grid.getCols()) {
        grid.setCell(r, c, 1);
        grid.setAge(r, c, 1);
      }
    }
  }
};
