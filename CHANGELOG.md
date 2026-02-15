# Changelog

All notable changes to Algo Nebula will be documented in this file.
Format based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [0.1.0] - 2026-02-14

### Added
- Project scaffolding: CMake + JUCE 8 + Gin + melatonin_blur + Signalsmith + DaisySP
- ASIO support for standalone build
- PluginProcessor with APVTS parameter layout (algorithm, scale, key, voices, waveshape, ambient, humanization, envelope, tuning)
- SmoothedValue master volume, pre-allocated buffers, CPU load metric
- NebulaLookAndFeel with Inter/JetBrains Mono fonts, gradient arc knobs, glow halos
- NebulaColours dark palette (indigo/pink accents)
- PluginEditor with dark gradient background, algorithm/scale/key/waveshape selectors, volume knob, CPU meter
- Headless test runner (build verification)
- Mandatory project docs
