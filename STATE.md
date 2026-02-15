# Project State

## Current Phase
Phase 3 — Clock + ScaleQuantizer + Microtuning (`v0.3.0`)

## Build Status
- VST3: Builds successfully (Release)
- Standalone: Builds successfully (Release)
- Tests: 52/52 passing (pure C++, no JUCE dependency)

## Key Classes
| Class | File | Status |
|-------|------|--------|
| `AlgoNebulaProcessor` | `src/PluginProcessor.h/.cpp` | Clock-driven stepping, engine + quantizer + tuning integrated |
| `AlgoNebulaEditor` | `src/PluginEditor.h/.cpp` | Dark UI with selectors, volume knob, CPU meter |
| `NebulaLookAndFeel` | `src/ui/NebulaLookAndFeel.h/.cpp` | Gradient arc knobs, glow, Inter/JetBrains fonts |
| `NebulaColours` | `src/ui/NebulaColours.h` | Dark palette tokens |
| `CellularEngine` | `src/engine/CellularEngine.h` | Abstract interface (step/randomize/clear/getGrid) |
| `Grid` | `src/engine/Grid.h` | Fixed 32x64, toroidal wrap, age, double-buffer |
| `GameOfLife` | `src/engine/GameOfLife.h/.cpp` | 5 rule presets, bitmask lookup, xorshift64 PRNG |
| `CellEditQueue` | `src/engine/CellEditQueue.h` | Lock-free SPSC, 256 capacity, cache-line aligned |
| `ScaleQuantizer` | `src/engine/ScaleQuantizer.h` | 15 scales, 12 root keys, O(1) array lookup |
| `Microtuning` | `src/engine/Microtuning.h` | 12-TET / Just / Pythagorean, adjustable A4, 128-note tables |
| `ClockDivider` | `src/engine/ClockDivider.h` | Integer sample counting, 6 divisions, swing (50-75%) |

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
- Mutation survival rate: 0% (all 6 mutations caught across phases)
