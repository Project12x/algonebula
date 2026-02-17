# Project State

## Current Phase
Phase 4.5 — Playability Fixes (`v0.4.5`)

## Build Status
- VST3: Builds successfully (Release)
- Standalone: Builds successfully (Release)
- Tests: 75/75 passing (pure C++, no JUCE dependency)

## Key Classes
| Class | File | Status |
|-------|------|--------|
| `AlgoNebulaProcessor` | `src/PluginProcessor.h/.cpp` | BPM param, MIDI keyboard state, note-on reseed, auto-reseed, algorithm->rule mapping, per-column waveshape |
| `AlgoNebulaEditor` | `src/PluginEditor.h/.cpp` | BPM knob, MidiKeyboardComponent, full control layout, v0.4.5 |
| `NebulaLookAndFeel` | `src/ui/NebulaLookAndFeel.h/.cpp` | Gradient arc knobs, glow, Inter/JetBrains fonts |
| `NebulaColours` | `src/ui/NebulaColours.h` | Dark palette tokens |
| `GridComponent` | `src/ui/GridComponent.h` | Real-time grid visualization, click-to-toggle cells |
| `CellularEngine` | `src/engine/CellularEngine.h` | Abstract interface (step/randomize/clear/getGrid) |
| `Grid` | `src/engine/Grid.h` | Fixed 32x64, toroidal wrap, age, double-buffer |
| `GameOfLife` | `src/engine/GameOfLife.h/.cpp` | 5 rule presets, bitmask lookup, xorshift64 PRNG |
| `CellEditQueue` | `src/engine/CellEditQueue.h` | Lock-free SPSC, 256 capacity, cache-line aligned |
| `ScaleQuantizer` | `src/engine/ScaleQuantizer.h` | 15 scales, 12 root keys, O(1) array lookup |
| `Microtuning` | `src/engine/Microtuning.h` | 12-TET / Just / Pythagorean, adjustable A4, 128-note tables |
| `ClockDivider` | `src/engine/ClockDivider.h` | Integer sample counting, 6 divisions, swing (50-75%) |
| `PolyBLEPOscillator` | `src/engine/PolyBLEPOscillator.h` | 8 waveshapes, polynomial bandlimited step, pulse width |
| `AHDSREnvelope` | `src/engine/AHDSREnvelope.h` | 5-stage envelope, linear ramps, retrigger from current level |
| `SVFilter` | `src/engine/SVFilter.h` | State-variable filter, 4 modes, Cytomic topology |
| `NoiseLayer` | `src/engine/NoiseLayer.h` | xorshift64 white noise, level control |
| `SubOscillator` | `src/engine/SubOscillator.h` | -1/-2 octave sine sub, level control |
| `SynthVoice` | `src/engine/SynthVoice.h` | Composite voice (osc+sub+noise+env+filter), stereo pan |

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
- Mutation survival rate: 0% (all 10 mutations caught across phases)

## Recent Changes (v0.4.5)
- BPM parameter (40-300) with UI knob
- MIDI keyboard component for virtual input
- MIDI note-on triggers grid reseed
- Auto-reseed after 8 stagnant generations
- Algorithm selector maps to GoL rule presets
- Per-column waveshape diversity (base + col offset mod 8)
