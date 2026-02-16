#pragma once

#include "../PluginProcessor.h"
#include "NebulaColours.h"
#include <juce_gui_basics/juce_gui_basics.h>

/// Renders the Game of Life grid with age-based cell coloring.
/// Click to toggle cells via the processor's CellEditQueue.
class GridComponent : public juce::Component, private juce::Timer {
public:
  explicit GridComponent(AlgoNebulaProcessor &p) : processor(p) {
    startTimerHz(20); // 20 FPS refresh
  }

  void paint(juce::Graphics &g) override {
    const auto &grid = processor.getGridSnapshot();
    const int rows = grid.getRows();
    const int cols = grid.getCols();

    if (rows <= 0 || cols <= 0)
      return;

    const float cellW = static_cast<float>(getWidth()) / cols;
    const float cellH = static_cast<float>(getHeight()) / rows;
    const float cellSize = std::min(cellW, cellH);
    const float gap = 1.0f;

    // Center the grid
    const float totalW = cellSize * cols;
    const float totalH = cellSize * rows;
    const float offsetX = (getWidth() - totalW) * 0.5f;
    const float offsetY = (getHeight() - totalH) * 0.5f;

    // Background
    g.setColour(NebulaColours::bg_deepest);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);

    for (int r = 0; r < rows; ++r) {
      for (int c = 0; c < cols; ++c) {
        float x = offsetX + c * cellSize + gap * 0.5f;
        float y = offsetY + r * cellSize + gap * 0.5f;
        float w = cellSize - gap;
        float h = cellSize - gap;

        uint8_t cellState = grid.getCell(r, c);

        if (cellState > 0) {
          // Age-based color: new(indigo) -> mid(purple) -> old(pink)
          uint8_t age = static_cast<uint8_t>(grid.getAge(r, c));
          float ageFrac = std::min(age / 20.0f, 1.0f);

          juce::Colour cellColour;
          if (ageFrac < 0.5f)
            cellColour = NebulaColours::cell_new.interpolatedWith(
                NebulaColours::cell_mid, ageFrac * 2.0f);
          else
            cellColour = NebulaColours::cell_mid.interpolatedWith(
                NebulaColours::cell_old, (ageFrac - 0.5f) * 2.0f);

          g.setColour(cellColour);
          g.fillRoundedRectangle(x, y, w, h, 2.0f);

          // Subtle glow on recently born cells
          if (age <= 2) {
            g.setColour(cellColour.withAlpha(0.3f));
            g.fillRoundedRectangle(x - 1, y - 1, w + 2, h + 2, 3.0f);
          }
        } else {
          // Dead cell — dark background
          g.setColour(NebulaColours::bg_surface);
          g.fillRoundedRectangle(x, y, w, h, 2.0f);
        }
      }
    }

    // Generation counter
    g.setColour(NebulaColours::text_dim);
    g.setFont(11.0f);
    g.drawText("Gen " + juce::String(processor.getGeneration()),
               getLocalBounds().removeFromBottom(16),
               juce::Justification::centredRight);
  }

  void mouseDown(const juce::MouseEvent &event) override {
    toggleCellAt(event.position);
  }

  void mouseDrag(const juce::MouseEvent &event) override {
    toggleCellAt(event.position);
  }

private:
  AlgoNebulaProcessor &processor;
  int lastToggledRow = -1;
  int lastToggledCol = -1;

  void timerCallback() override { repaint(); }

  void toggleCellAt(juce::Point<float> pos) {
    const auto &grid = processor.getGridSnapshot();
    const int rows = grid.getRows();
    const int cols = grid.getCols();

    if (rows <= 0 || cols <= 0)
      return;

    const float cellSize = std::min(static_cast<float>(getWidth()) / cols,
                                    static_cast<float>(getHeight()) / rows);
    const float totalW = cellSize * cols;
    const float totalH = cellSize * rows;
    const float offsetX = (getWidth() - totalW) * 0.5f;
    const float offsetY = (getHeight() - totalH) * 0.5f;

    int col = static_cast<int>((pos.x - offsetX) / cellSize);
    int row = static_cast<int>((pos.y - offsetY) / cellSize);

    if (row < 0 || row >= rows || col < 0 || col >= cols)
      return;

    // Avoid rapid repeat toggles on same cell during drag
    if (row == lastToggledRow && col == lastToggledCol)
      return;

    lastToggledRow = row;
    lastToggledCol = col;

    // Toggle: if alive -> kill, if dead -> birth
    uint8_t currentState = grid.getCell(row, col);
    uint8_t newState = (currentState > 0) ? 0 : 1;

    processor.getCellEditQueue().push(row, col, newState);
  }

  void mouseUp(const juce::MouseEvent &) override {
    lastToggledRow = -1;
    lastToggledCol = -1;
  }

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GridComponent)
};
