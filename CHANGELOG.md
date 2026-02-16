# Changelog

All notable changes to Algo Nebula will be documented in this file.
Format based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [0.4.0] - 2026-02-16

### Added

- `PolyBLEPOscillator`: 8 waveshapes (Sine, Triangle, Saw, Pulse, SineOct, FifthStack, Pad, Bell), polynomial bandlimited step, pulse width control
- `AHDSREnvelope`: 5-stage envelope (Attack, Hold, Decay, Sustain, Release), linear ramps, retrigger from current level
- `SVFilter`: State-variable filter (LP, HP, BP, Notch), Cytomic topology, cutoff/resonance, RT-safe coefficient updates
- `NoiseLayer`: xorshift64 white noise with level control
- `SubOscillator`: -1/-2 octave pure sine sub with level control
- `SynthVoice`: Composite voice class (osc + sub + noise + envelope + filter), stereo panning
- 9 new APVTS parameters: hold, decay, sustain, filterCutoff, filterRes, filterMode, noiseLevel, subLevel, subOctave
- Voice triggering in processBlock: grid-to-note mapping via ScaleQuantizer/Microtuning, 8-voice polyphony, voice stealing
- 23 new headless tests: 7 oscillator, 5 envelope, 4 filter, 3 voice, 4 mutation
- Total test count: 75 (all passing, 0% mutation survival)

## [0.3.0] - 2026-02-15

### Added

- `ScaleQuantizer`: 15 scales (Chromatic, Major, Minor, modes, pentatonics, blues, whole-tone, harmonic/melodic minor), 12 root keys, O(1) array lookup
- `Microtuning`: 3 tuning systems (12-TET, Just Intonation, Pythagorean), adjustable A4 reference, pre-computed 128-note frequency tables
- `ClockDivider`: integer sample counting, 6 divisions (whole to 1/32), swing (50-75%), block-level processing
- Clock-driven engine stepping in `processBlock` (replaces manual stepping)
- 22 new headless tests: 5 ScaleQuantizer, 6 Microtuning, 5 ClockDivider, 3 integration, 3 mutation
- Total test count: 52 (all passing)

## [0.2.0] - 2026-02-15

### Added

- `CellularEngine` abstract interface for all cellular automata
- `Grid` data structure: fixed-size 32x64, toroidal wrapping, cell age tracking, double-buffer support
- `GameOfLife` implementation: 5 rule presets (Classic B3/S23, HighLife B36/S23, DayAndNight B3678/S34678, Seeds B2/S, Ambient B3/S2345), bitmask rule lookup, xorshift64 PRNG seeding, scratch grid for allocation-free stepping
- `CellEditQueue` lock-free SPSC ring buffer (256 capacity, cache-line aligned atomics) for UI -> audio cell edits
- Engine integrated into PluginProcessor: cell edit draining, grid snapshot for GL/UI thread
- 30 headless tests: 9 Grid, 10 GoL correctness, 4 rule variants, 4 SPSC queue, 3 mutation tests
- Test target decoupled from JUCE (pure C++ for fast compilation)

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
