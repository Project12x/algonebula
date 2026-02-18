# Project State

## Current Phase
Phase 7 — Synth Expansion + DSP (`v0.7.0`) -- COMPLETE

## Build Status
- VST3: Builds successfully (Release)
- Standalone: Builds successfully (Release)
- Tests: 89/89 passing (pure C++, no JUCE dependency)

## Key Classes
| Class | File | Status |
|-------|------|--------|
| `AlgoNebulaProcessor` | `src/PluginProcessor.h/.cpp` | Multi-engine factory, density modulation, musicality params, SmoothedValue on 6 continuous params, adjustable grid size, 3 DSP effects chain, stereo width |
| `AlgoNebulaEditor` | `src/PluginEditor.h/.cpp` | Full control layout: envelope, filter, mix, clock, tuning, ambient, humanize, global sections. Grid size dropdown in transport strip |
| `NebulaLookAndFeel` | `src/ui/NebulaLookAndFeel.h/.cpp` | Gradient arc knobs, glow, Inter/JetBrains fonts |
| `NebulaColours` | `src/ui/NebulaColours.h` | Dark palette tokens + 6 engine visualization tokens |
| `GridComponent` | `src/ui/GridComponent.h` | Engine-aware visualization (per-engine color palettes), click-to-toggle cells |
| `CellularEngine` | `src/engine/CellularEngine.h` | Abstract interface (step/randomize/clear/getGrid/getType/getName) |
| `Grid` | `src/engine/Grid.h` | 512x512 max, std::vector heap storage, toroidal wrap, age, birth tracking, double-buffer |
| `GameOfLife` | `src/engine/GameOfLife.h/.cpp` | 5 rule presets, bitmask lookup, xorshift64 PRNG |
| `BriansBrainEngine` | `src/engine/BriansBrainEngine.h` | 3-state (alive/dying/dead) automaton |
| `CyclicCA` | `src/engine/CyclicCA.h` | N-state rotating spiral automaton |
| `ReactionDiffusion` | `src/engine/ReactionDiffusion.h` | Gray-Scott model, std::vector float fields |
| `LeniaEngine` | `src/engine/LeniaEngine.h` | Continuous-state, Gaussian kernel, std::vector fields |
| `ParticleSwarm` | `src/engine/ParticleSwarm.h` | Agent-based particle trails, std::vector trail field |
| `BrownianField` | `src/engine/BrownianField.h` | Multi-walker random walk, std::vector energy field |
| `ScaleQuantizer` | `src/engine/ScaleQuantizer.h` | 15 scales, 12 root keys, O(1) array lookup |
| `Microtuning` | `src/engine/Microtuning.h` | 12-TET / Just / Pythagorean, adjustable A4, 128-note tables |
| `ClockDivider` | `src/engine/ClockDivider.h` | Integer sample counting, 6 divisions, swing (50-75%) |
| `PolyBLEPOscillator` | `src/engine/PolyBLEPOscillator.h` | 8 waveshapes, bandlimited step, pulse width |
| `AHDSREnvelope` | `src/engine/AHDSREnvelope.h` | 5-stage envelope, linear ramps, retrigger |
| `SVFilter` | `src/engine/SVFilter.h` | State-variable filter, 4 modes, Cytomic topology |
| `SynthVoice` | `src/engine/SynthVoice.h` | Composite voice (osc+sub+noise+env+filter), stereo pan, grid position tracking |
| `CellEditQueue` | `src/engine/CellEditQueue.h` | Lock-free SPSC, 256 capacity, cache-line aligned |
| `FactoryPresets` | `src/engine/FactoryPresets.h` | 11 presets (musical, experimental, utility categories) |
| `FactoryPatternLibrary` | `src/engine/FactoryPatternLibrary.h` | 5 GoL seed patterns (Glider, LWSS, R-Pentomino, Pulsar, Gosper Gun) |
| `StereoChorus` | `src/dsp/StereoChorus.h` | Stereo chorus, LFO-modulated delay, wet/dry mix |
| `StereoDelay` | `src/dsp/StereoDelay.h` | Stereo delay with cross-feedback, per-sample processing |
| `PlateReverb` | `src/dsp/PlateReverb.h` | Dattorro plate reverb, 4 allpass + 2 comb, decay/damping |

## Recent Changes (v0.7.0)
- Grid max expanded to 512x512, voices to 64
- Grid + 4 engine classes converted to std::vector (heap allocation)
- 3 header-only DSP effects created and integrated into processBlock
- 10 new APVTS parameters for stereo width and effects
- Stereo width scales grid-position panning
- 89 tests still passing
