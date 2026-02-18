# Project State

## Current Phase
Phase 7 complete (`v0.7.1`) — Next: Phase 8 (Effects Expansion) or Phase 7b (MIDI I/O)

## Build Status
- VST3: Builds successfully (Release)
- Standalone: Builds successfully (Release)
- Tests: 89/89 passing (pure C++, no JUCE dependency)
- Repository: https://github.com/Project12x/algonebula

## Version History
| Version | Tag | Description |
|---------|-----|-------------|
| v0.1.0 | `v0.1.0` | Skeleton + UI foundation |
| v0.2.0 | `v0.2.0` | CellularEngine + GameOfLife + SPSC queue |
| v0.3.0 | `v0.3.0` | Scale quantizer + clock divider + microtuning |
| v0.4.0 | `v0.4.0` | PolyBLEP synth voices + AHDSR + SVF filter |
| v0.4.5 | — | BPM, MIDI keyboard, auto-reseed, algorithm switching |
| v0.5.0 | — | 7 CA engines, musicality, anti-cacophony, grid visualization |
| v0.5.5 | — | Gate time, strum spread, engine-specific intensity |
| v0.6.0 | `v0.6.0` | Grid persistence, factory patterns, freeze mode |
| v0.7.0 | `v0.7.0` | 512x512 grid, 64 voices, DSP effects chain |
| v0.7.1 | `v0.7.1` | Effects stabilization (feedback removal, limiter, parallel FX) |

## Completed Phases
- Phase 1: Skeleton + UI Foundation
- Phase 2: CellularEngine + Game of Life
- Phase 3: Scale Quantizer + Clock + Microtuning
- Phase 4: PolyBLEP Synth Voices
- Phase 4.5: Playability Fixes
- Phase 5: Integration + Musicality (7 CA engines)
- Phase 5.5: Musicality Phase 2 (gate time, strum, engine intensity)
- Phase 6: Grid Persistence + Seeding
- Phase 7: Synth Expansion + DSP Effects
- Phase 9/10: Merged into Phase 5 (additional CA engines)

## Key Classes
| Class | File | Status |
|-------|------|--------|
| `AlgoNebulaProcessor` | `src/PluginProcessor.h/.cpp` | Multi-engine factory, parallel FX chain, safety limiter, density modulation, state serialization |
| `AlgoNebulaEditor` | `src/PluginEditor.h/.cpp` | Full control layout, FX popout, grid size dropdown, freeze button, pattern selector |
| `NebulaLookAndFeel` | `src/ui/NebulaLookAndFeel.h/.cpp` | Gradient arc knobs, glow, Inter/JetBrains fonts |
| `NebulaColours` | `src/ui/NebulaColours.h` | Dark palette tokens + 6 engine visualization tokens |
| `GridComponent` | `src/ui/GridComponent.h` | Engine-aware visualization (per-engine color palettes), click-to-toggle cells |
| `EffectsPanel` | `src/ui/EffectsPanel.h` | Non-modal FX popout, 10 knobs in 4 sections |
| `CellularEngine` | `src/engine/CellularEngine.h` | Abstract interface (step/randomize/clear/getGrid/getType/getName/getCellIntensity/cellActivated) |
| `Grid` | `src/engine/Grid.h` | 512x512 max, std::vector heap storage, toroidal wrap, age, birth tracking, double-buffer |
| `GameOfLife` | `src/engine/GameOfLife.h/.cpp` | 5 rule presets, bitmask lookup, xorshift64 PRNG |
| `BriansBrainEngine` | `src/engine/BriansBrainEngine.h` | 3-state (alive/dying/dead) automaton |
| `CyclicCA` | `src/engine/CyclicCA.h` | N-state rotating spiral automaton |
| `ReactionDiffusion` | `src/engine/ReactionDiffusion.h` | Gray-Scott model, std::vector float fields |
| `LeniaEngine` | `src/engine/LeniaEngine.h` | Continuous-state, Gaussian kernel, std::vector fields |
| `ParticleSwarm` | `src/engine/ParticleSwarm.h` | Agent-based particle trails, std::vector trail field |
| `BrownianField` | `src/engine/BrownianField.h` | Multi-walker random walk, std::vector energy field |
| `ScaleQuantizer` | `src/engine/ScaleQuantizer.h` | 15 scales, 12 root keys, O(1) array lookup, consonance filter |
| `Microtuning` | `src/engine/Microtuning.h` | 12-TET / Just / Pythagorean, adjustable A4, 128-note tables |
| `ClockDivider` | `src/engine/ClockDivider.h` | Integer sample counting, 6 divisions, swing (50-75%) |
| `PolyBLEPOscillator` | `src/engine/PolyBLEPOscillator.h` | 8 waveshapes, bandlimited step, pulse width |
| `AHDSREnvelope` | `src/engine/AHDSREnvelope.h` | 5-stage envelope, linear ramps, retrigger |
| `SVFilter` | `src/engine/SVFilter.h` | State-variable filter, 4 modes, Cytomic topology |
| `SynthVoice` | `src/engine/SynthVoice.h` | Composite voice (osc+sub+noise+env+filter), stereo pan, gate time, strum spread |
| `CellEditQueue` | `src/engine/CellEditQueue.h` | Lock-free SPSC, 256 capacity, cache-line aligned |
| `FactoryPresets` | `src/engine/FactoryPresets.h` | 16 presets (musical, experimental, utility categories) |
| `FactoryPatternLibrary` | `src/engine/FactoryPatternLibrary.h` | 5 GoL seed patterns (Glider, LWSS, R-Pentomino, Pulsar, Gosper Gun) |
| `StereoChorus` | `src/dsp/StereoChorus.h` | Stereo chorus, LFO-modulated delay, no internal feedback |
| `StereoDelay` | `src/dsp/StereoDelay.h` | Single-echo stereo delay, no feedback/cross-feed |
| `PlateReverb` | `src/dsp/PlateReverb.h` | Dattorro plate reverb, reduced tank coupling (decay*0.5) |

## Recent Changes (v0.7.1)
- Effects chain restructured to parallel send/return architecture
- Removed internal feedback from Chorus and Delay (continuous input safety)
- Reduced PlateReverb tank cross-coupling by 50%
- Added juce::dsp::Limiter as final output stage (-1dB ceiling)
- Input soft-clipping and wet signal attenuation
- Tighter sanitize clamps on all effects (+/-1.5)
- All effect parameter ranges reduced to safe values
