#pragma once

#include "CellularEngine.h"
#include "Grid.h"
#include <cstdint>

/// Game of Life implementation with 5 rule presets, toroidal wrapping,
/// cell age tracking, and seeded random initialization.
///
/// Rule presets use Birth/Survival notation:
///   Classic (B3/S23), High Life (B36/S23), Day & Night (B3678/S34678),
///   Seeds (B2/S), Ambient (B3/S2345)
class GameOfLife final : public CellularEngine {
public:
  /// Rule preset enumeration.
  enum class RulePreset : int {
    Classic = 0, // B3/S23
    HighLife,    // B36/S23
    DayAndNight, // B3678/S34678
    Seeds,       // B2/S (no survival)
    Ambient,     // B3/S2345 (high survival, slow decay)
    Count
  };

  /// Construct with grid dimensions and rule preset.
  explicit GameOfLife(int rows = 12, int cols = 16,
                      RulePreset preset = RulePreset::Classic);

  // --- CellularEngine interface ---
  void step() override;
  void randomize(uint64_t seed, float density) override;
  void clear() override;
  const Grid &getGrid() const override { return grid; }
  Grid &getGridMutable() override { return grid; }
  uint64_t getGeneration() const override { return generation; }
  const char *getName() const override { return "Game of Life"; }

  // --- GoL-specific ---
  void setRulePreset(RulePreset preset);
  RulePreset getRulePreset() const { return currentPreset; }

  /// Load a known pattern at given offset.
  /// Pattern data is a vector of {row, col} offsets relative to origin.
  void loadPattern(const int (*cells)[2], int count, int originRow,
                   int originCol);

private:
  /// Count live neighbors (toroidal) for cell at (row, col).
  int countNeighbors(int row, int col) const;

  /// Apply birth/survival rules.
  /// birthRule and survivalRule are bitmasks: bit N set = N neighbors triggers.
  uint16_t birthRule = 0;
  uint16_t survivalRule = 0;

  void applyPreset(RulePreset preset);

  Grid grid;
  Grid scratch; // Pre-allocated scratch grid for next generation
  uint64_t generation = 0;
  RulePreset currentPreset = RulePreset::Classic;
};
