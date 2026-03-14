# Project State

## Current Phase
Phase 13c complete — GPU compute integrated, float bridge stabilized. Next: grid conversion toggle, GPU Brownian crash, or Phase 11 (Arp + Vibrato)

## Build Status
- VST3: Builds successfully (Release)
- Standalone: Builds successfully (Release)
- Tests: 107/107 passing (pure C++, no JUCE dependency)
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
| v0.8.0 | `v0.8.0` | 6 new effects, StereoEffect base, EffectChain, SafetyProcessor |
| v0.9.0 | `v0.9.0` | Effect toggles, modulation matrix, CA energy stability |
| v0.13.0 | — | GPU compute integration, float bridge, expanded grid sizes |

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
- Phase 8: Effects Chain Expansion (6 new effects, StereoEffect, EffectChain, SafetyProcessor)
- Phase 9: Effect Toggles, Modulation Matrix, CA Energy Stability
- Phase 13a: CMake + ghostsun_render build integration
- Phase 13b: GPU compute adapters (7 WGSL shaders + ComputeSimulation subclasses)
- Phase 13c: Processor integration (GpuGridBridge, float bridge, generation-gated convertToGrid)

## Key Classes
| Class | File | Status |
|-------|------|--------|
| `AlgoNebulaProcessor` | `src/PluginProcessor.h/.cpp` | Multi-engine factory, GPU/CPU dual path, 9 effects with toggles, 2 LFO mod matrix, float bridge integration |
| `AlgoNebulaEditor` | `src/PluginEditor.h/.cpp` | Full control layout, FX popout, grid size dropdown (12 sizes), GPU toggle + meter, freeze button |
| `GpuComputeManager` | `src/gpu/GpuComputeManager.h/.cpp` | Timer-driven GPU simulation loop, engine adapter management, async readback |
| `GpuGridBridge` | `src/gpu/GpuGridBridge.h` | Lock-free float double-buffer, generation-gated convertToGrid, intensity-as-age mapping |
| `GridComponent` | `src/ui/GridComponent.h` | Engine-aware visualization (per-engine color palettes), click-to-toggle cells |
| `NebulaLookAndFeel` | `src/ui/NebulaLookAndFeel.h/.cpp` | Gradient arc knobs, glow, Inter/JetBrains fonts |
| `NebulaColours` | `src/ui/NebulaColours.h` | Dark palette tokens + 6 engine visualization tokens |
| `EffectsPanel` | `src/ui/EffectsPanel.h` | Non-modal FX popout, 9 toggles, trigger budget, 2 LFO sections, 12 effect sections |
| `ModLFO` | `src/dsp/ModLFO.h` | 5-shape modulation LFO (sine/tri/saw/square/S&H), per-sample and per-block tick |
| `StereoEffect` | `src/dsp/StereoEffect.h` | Abstract base for all effects (init/process/reset/mix/bypass) |
| `EffectChain` | `src/dsp/EffectChain.h` | 16-slot effects manager with parallel processing |
| `SafetyProcessor` | `src/dsp/SafetyProcessor.h` | DC block (5Hz HP) + brickwall limiter (-0.3dBFS) |
| `CellularEngine` | `src/engine/CellularEngine.h` | Abstract interface + getDefaultTriggerBudget() + getGainScale() |
| `Grid` | `src/engine/Grid.h` | 1280x1280 max, std::vector heap storage, toroidal wrap, age, birth tracking |
| `GameOfLife` | `src/engine/GameOfLife.h/.cpp` | 5 rule presets, bitmask lookup, xorshift64 PRNG |
| `BriansBrainEngine` | `src/engine/BriansBrain.h` | 3-state (alive/dying/dead) automaton |
| `CyclicCA` | `src/engine/CyclicCA.h` | N-state rotating spiral automaton (budget=5, gain=0.5) |
| `ReactionDiffusion` | `src/engine/ReactionDiffusion.h` | Gray-Scott model (budget=4, gain=0.4) |
| `LeniaEngine` | `src/engine/LeniaEngine.h` | Continuous-state, Gaussian kernel (budget=4, gain=0.4) |
| `ParticleSwarm` | `src/engine/ParticleSwarm.h` | Agent-based particle trails (budget=4, gain=0.5) |
| `BrownianField` | `src/engine/BrownianField.h` | Multi-walker random walk (budget=6, gain=0.5) |
| `ScaleQuantizer` | `src/engine/ScaleQuantizer.h` | 15 scales, 12 root keys, O(1) array lookup |
| `Microtuning` | `src/engine/Microtuning.h` | 12-TET / Just / Pythagorean, adjustable A4 |
| `ClockDivider` | `src/engine/ClockDivider.h` | Integer sample counting, 6 divisions, swing |
| `PolyBLEPOscillator` | `src/engine/PolyBLEPOscillator.h` | 8 waveshapes, bandlimited step, pulse width |
| `SynthVoice` | `src/engine/SynthVoice.h` | Composite voice (osc+sub+noise+env+filter), stereo pan, gate time |
| `FactoryPresets` | `src/engine/FactoryPresets.h` | 16 presets with effect toggles and trigger budget |

## Recent Changes (v0.13.0)
- GPU compute integration via ghostsun_render (WebGPU/Dawn)
- GpuComputeManager: timer-driven GPU simulation with async readback
- GpuGridBridge: float-only double-buffer with lock-free reads
- 7 GPU engine adapters (WGSL compute shaders)
- GPU toggle (APVTS `gpuAccel`, default OFF) + GPU step time meter
- Grid sizes expanded to 12 options (8x12 through 1280x1280)
- Float bridge crash fix: generation-gated convertToGrid
- Continuous engine visualization: float intensity mapped to age for rendering

## Known Issues
- GPU crash on Brownian engine (needs investigation)
- Grid conversion toggle (binary vs. float mode) not yet implemented
- Lint errors in IDE from missing JUCE headers (false positives, builds fine)
