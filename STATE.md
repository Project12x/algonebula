# Project State

## Current Phase
Phase 5.5 — Musicality Phase 2 (`v0.5.0`) -- COMPLETE

## Build Status
- VST3: Builds successfully (Release)
- Standalone: Builds successfully (Release)
- Tests: 86/86 passing (pure C++, no JUCE dependency)

## Key Classes
| Class | File | Status |
|-------|------|--------|
| `AlgoNebulaProcessor` | `src/PluginProcessor.h/.cpp` | Multi-engine factory, density modulation, musicality params, SmoothedValue on 6 continuous params, adjustable grid size |
| `AlgoNebulaEditor` | `src/PluginEditor.h/.cpp` | Full control layout: envelope, filter, mix, clock, tuning, ambient, humanize, global sections. Grid size dropdown in transport strip |
| `NebulaLookAndFeel` | `src/ui/NebulaLookAndFeel.h/.cpp` | Gradient arc knobs, glow, Inter/JetBrains fonts |
| `NebulaColours` | `src/ui/NebulaColours.h` | Dark palette tokens + 6 engine visualization tokens |
| `GridComponent` | `src/ui/GridComponent.h` | Engine-aware visualization (per-engine color palettes), click-to-toggle cells |
| `CellularEngine` | `src/engine/CellularEngine.h` | Abstract interface (step/randomize/clear/getGrid/getType/getName) |
| `Grid` | `src/engine/Grid.h` | Dynamic resize (8x12 to 24x32), toroidal wrap, age, birth tracking, double-buffer |
| `GameOfLife` | `src/engine/GameOfLife.h/.cpp` | 5 rule presets, bitmask lookup, xorshift64 PRNG |
| `BriansBrainEngine` | `src/engine/BriansBrainEngine.h` | 3-state (alive/dying/dead) automaton |
| `CyclicCA` | `src/engine/CyclicCA.h` | N-state rotating spiral automaton |
| `ReactionDiffusion` | `src/engine/ReactionDiffusion.h` | Gray-Scott model with float fields |
| `LeniaEngine` | `src/engine/LeniaEngine.h` | Continuous-state, Gaussian kernel, growth function |
| `ParticleSwarm` | `src/engine/ParticleSwarm.h` | Agent-based particle trails |
| `BrownianField` | `src/engine/BrownianField.h` | Multi-walker random walk energy deposition |
| `ScaleQuantizer` | `src/engine/ScaleQuantizer.h` | 15 scales, 12 root keys, O(1) array lookup |
| `Microtuning` | `src/engine/Microtuning.h` | 12-TET / Just / Pythagorean, adjustable A4, 128-note tables |
| `ClockDivider` | `src/engine/ClockDivider.h` | Integer sample counting, 6 divisions, swing (50-75%) |
| `PolyBLEPOscillator` | `src/engine/PolyBLEPOscillator.h` | 8 waveshapes, bandlimited step, pulse width |
| `AHDSREnvelope` | `src/engine/AHDSREnvelope.h` | 5-stage envelope, linear ramps, retrigger |
| `SVFilter` | `src/engine/SVFilter.h` | State-variable filter, 4 modes, Cytomic topology |
| `SynthVoice` | `src/engine/SynthVoice.h` | Composite voice (osc+sub+noise+env+filter), stereo pan, grid position tracking |
| `CellEditQueue` | `src/engine/CellEditQueue.h` | Lock-free SPSC, 256 capacity, cache-line aligned |
| `FactoryPresets` | `src/engine/FactoryPresets.h` | 11 presets (musical, experimental, utility categories) |

## Test Metrics
- Grid tests: 9 (basics, wrapping, age, copy, equality)
- GoL correctness: 10 (Blinker, Block, Beehive, Glider, generation, age, seeding, density)
- Rule variants: 4 (HighLife, Seeds, Ambient, toroidal)
- SPSC queue: 4 (push/pop, full, drain, bounded drain)
- Phase 2 mutation: 3 (birth rule flip, survival disabled, age reset)
- ScaleQuantizer: 5 (all 15 scales, 12 roots, out-of-scale check, MIDI clamp, pentatonic)
- Microtuning: 6 (12-TET, Just P5, Pythagorean, ref pitch, all systems A4, cents offset)
- ClockDivider: 5 (quarter at 120BPM, all divisions, swing, no-swing, buffer accuracy)
- Integration: 3 (clock drives GoL, quantizer+GoL, transport pause/resume)
- Phase 3 mutation: 3 (Dorian interval, Just P5 offset, clock comparator)
- Oscillator: 7 (sine accuracy, saw/square range, pulse symmetry, anti-aliasing, output range, frequency accuracy)
- Envelope: 5 (attack timing, hold/decay/sustain, release, note-off during attack, retrigger)
- Filter: 4 (LP response, HP response, resonance, stability at max Q)
- SynthVoice: 3 (full chain output, 8-voice polyphony, sub tracking)
- Phase 4 mutation: 4 (PolyBLEP presence, instant attack, cutoff offset, sub octave division)
- CA Engine tests: 7 (Brian's Brain activity, Cyclic step, R-D fields, Lenia continuous state, Particle trails, Brownian deposits, engine type ID)
- Mutation survival rate: 0% (all mutations caught)

## Recent Changes (v0.5.0)
- 7 CA engines: GoL, Brian's Brain, Cyclic CA, Reaction-Diffusion, Lenia, Particle Swarm, Brownian Field
- Engine-aware GridComponent visualization with per-engine color palettes
- Density-driven dynamics: grid density modulates voice gain and filter cutoff
- Wired musicality params: noteProbability, velocityHumanize, melodicInertia
- Tuned defaults for less cacophony: noteProbability 0.5, voiceCount 3, attack 0.8s, melodicInertia 0.5
- Adjustable grid size: Small (8x12), Medium (12x16), Large (16x24), XL (24x32)
- Grid size dropdown in transport strip UI
- 11 factory presets with corrected algorithm indices
- 2 new presets: Tidal Lenia, Chemical Garden
- Hover tooltips on all 31 UI controls
- SmoothedValue on filter cutoff, resonance, noise, sub level, density gain (20ms ramp)
- Anti-cacophony: consonance filter, max triggers/step, rest probability, pitch gravity, density-adaptive voices
