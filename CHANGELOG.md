# Changelog

All notable changes to Algo Nebula will be documented in this file.
Format based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [0.5.0] - 2026-02-17

### Added

- 6 distinct CA engines: Brian's Brain, Cyclic CA, Reaction-Diffusion, Lenia, Particle Swarm, Brownian Field
- Each engine is a unique computational model with its own data structures (not just GoL rule variants)
- Engine-aware GridComponent visualization: each engine renders with distinct color palette
  - Brian's Brain: cyan(on)/amber(dying) 3-state display
  - Cyclic CA: HSL hue wheel mapped from cell state
  - Reaction-Diffusion, Lenia, Swarm, Brownian: intensity heatmaps from native data
- Density-driven dynamics modulation: grid density scales voice gain (dense=softer) and filter cutoff
- Engine name displayed alongside generation counter in status bar
- 6 new NebulaColours tokens for engine-specific visualization
- `getEngine()` and `getDensityGain()` accessors on PluginProcessor
- 7 new CA engine unit tests (82 total, all passing)
- Adjustable grid resolution: Small (8x12), Medium (12x16), Large (16x24), XL (24x32) via APVTS parameter
- Grid size dropdown in transport strip UI
- Wired `noteProbability`, `velocityHumanize`, `melodicInertia` params into voice triggering
- "Tidal Lenia" preset showcasing Lenia engine with slow waveform-rich sound
- "Chemical Garden" preset showcasing Reaction-Diffusion with evolving textures
- "Neural Flicker" preset for Brian's Brain (fast percussive 3-state pulses)
- "Spectrum Cycle" preset for Cyclic CA (whole-tone color wheel)
- "Swarm Murmuration" preset for Particle Swarm (flowing trails, just intonation)
- "Fog Machine" preset for Brownian Field (diffuse drones, Pythagorean tuning)
- Gate time: per-voice auto-noteOff countdown (staccato at &lt;1.0) driven by step interval
- Strum spread: per-column onset delay (0-50ms) for arpeggiation-like spread
- Engine-specific intensity: continuous engines modulate velocity from native float fields
- `getCellIntensity()` / `cellActivated()` virtual methods on CellularEngine base class
- `getStepIntervalSeconds()` convenience method on ClockDivider

### Changed

- Algorithm selector now creates genuinely different engine types (was GoL rule presets)
- PluginProcessor refactored to `unique_ptr<CellularEngine>` with factory pattern for engine switching
- Default `noteProbability` tuned from 1.0 to 0.5 (fewer simultaneous triggers)
- Default `voiceCount` tuned from 4 to 3 (less dense)
- Default `attack` tuned from 0.5s to 0.8s (softer entry)
- Default `melodicInertia` tuned from 0.3 to 0.5 (smoother melodies)
- Fixed stale algorithm indices in Dark Drone and Pulsing Seeds presets
- All factory presets now use wired musicality parameters effectively
- Voice triggering uses `cellActivated()` instead of raw `wasBorn()` for engine-aware activation
- 13 factory presets total (up from 9), covering all 7+1 engine types

## [0.4.5] - 2026-02-16

### Added

- BPM APVTS parameter (40-300, default 120) with rotary knob in Clock section
- `MidiKeyboardComponent` at bottom of editor for virtual MIDI input
- MIDI note-on reseed: playing a key reseeds the grid with note-derived RNG
- Auto-reseed: injects ~5 random cells after 8 stagnant generations (alive count unchanged)
- Algorithm selector now maps to GoL rule presets (Classic, HighLife, DayAndNight, Seeds, Ambient)
- Per-column waveshape: each grid column offsets the base waveshape, creating timbral variety
- Clock division and swing parameters read from APVTS and applied per processBlock

### Changed

- Window size increased to 1000x780 (was 1000x700) to accommodate keyboard
- Removed fixed aspect ratio constraint for freeform resizing
- Version bumped to v0.4.5 in editor display

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
