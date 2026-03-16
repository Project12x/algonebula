#pragma once

#include "../gpu/GpuComputeManager.h"
#include "NebulaColours.h"
#include <juce_gui_basics/juce_gui_basics.h>

/// Displays the 3D Lenia volume as a raymarched image.
/// Mouse drag controls camera orbit/elevation, wheel controls zoom.
/// Right-click cycles color modes.
class VolumeComponent : public juce::Component, private juce::Timer {
public:
  explicit VolumeComponent(GpuComputeManager &gpu) : gpuCompute(gpu) {
    startTimerHz(60); // 60 FPS refresh
  }

  void paint(juce::Graphics &g) override {
    // Background
    g.setColour(NebulaColours::bg_deepest);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);

    auto frame = gpuCompute.getVolumeFrame();
    if (frame.isValid()) {
      // Draw the raymarched volume
      g.drawImage(frame, getLocalBounds().toFloat(),
                  juce::RectanglePlacement::centred);
    } else {
      // Placeholder while GPU initializes
      g.setColour(NebulaColours::text_dim);
      g.setFont(14.0f);
      g.drawText("Lenia 3D - Initializing GPU...",
                 getLocalBounds(), juce::Justification::centred);
    }

    // Label
    g.setColour(NebulaColours::text_dim.withAlpha(0.6f));
    g.setFont(11.0f);
    juce::String info = "Lenia 3D | ";
    static const char *modeNames[] = {"Divine", "Heat", "Mono", "Nebula"};
    info += modeNames[colorMode_ % 4];
    info += " | " + juce::String(gpuCompute.getGpuStepMs(), 1) + "ms";
    g.drawText(info, getLocalBounds().removeFromBottom(18),
               juce::Justification::centredRight);
  }

  void mouseDown(const juce::MouseEvent &event) override {
    if (event.mods.isRightButtonDown()) {
      // Cycle color mode
      colorMode_ = (colorMode_ + 1) % 4;
      gpuCompute.setColorMode(colorMode_);
      return;
    }
    dragging_ = true;
    lastDragPos_ = event.position;
    autoOrbit_ = false;
  }

  void mouseDrag(const juce::MouseEvent &event) override {
    if (!dragging_) return;
    float dx = event.position.x - lastDragPos_.x;
    float dy = event.position.y - lastDragPos_.y;
    lastDragPos_ = event.position;

    orbit_ += dx * 0.01f;
    elevation_ = std::clamp(elevation_ - dy * 0.005f, -0.8f, 1.2f);

    gpuCompute.setCameraOrbit(orbit_);
    gpuCompute.setCameraElevation(elevation_);
  }

  void mouseUp(const juce::MouseEvent &) override {
    dragging_ = false;
  }

  void mouseDoubleClick(const juce::MouseEvent &) override {
    // Double-click re-enables auto-orbit
    autoOrbit_ = true;
  }

  void mouseWheelMove(const juce::MouseEvent &,
                      const juce::MouseWheelDetails &wheel) override {
    distance_ = std::clamp(distance_ - wheel.deltaY * 0.5f, 0.8f, 5.0f);
    gpuCompute.setCameraDistance(distance_);
  }

  void resized() override {
    // Tell GpuComputeManager the render size
    // Use component size as render target (not fullscreen)
    // No extra method needed - offscreen texture will be created on first render
  }

private:
  GpuComputeManager &gpuCompute;
  bool dragging_ = false;
  bool autoOrbit_ = true;
  juce::Point<float> lastDragPos_;
  float orbit_ = 0.0f;
  float elevation_ = 0.3f;
  float distance_ = 2.0f;
  int colorMode_ = 3; // nebula

  void timerCallback() override {
    if (autoOrbit_) {
      orbit_ += 0.008f; // slow auto-rotation
      gpuCompute.setCameraOrbit(orbit_);
    }
    repaint();
  }

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VolumeComponent)
};
