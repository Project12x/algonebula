# Project State

## Current Phase
Phase 2 — CellularEngine + Game of Life (`v0.2.0`)

## Build Status
- VST3: Builds successfully (Release)
- Standalone: Builds successfully (Release)
- Tests: 30/30 passing (pure C++, no JUCE dependency)

## Key Classes
| Class | File | Status |
|-------|------|--------|
| `AlgoNebulaProcessor` | `src/PluginProcessor.h/.cpp` | Engine integrated, cell edit drain, grid snapshot |
| `AlgoNebulaEditor` | `src/PluginEditor.h/.cpp` | Dark UI with selectors, volume knob, CPU meter |
| `NebulaLookAndFeel` | `src/ui/NebulaLookAndFeel.h/.cpp` | Gradient arc knobs, glow, Inter/JetBrains fonts |
| `NebulaColours` | `src/ui/NebulaColours.h` | Dark palette tokens |
| `CellularEngine` | `src/engine/CellularEngine.h` | Abstract interface (step/randomize/clear/getGrid) |
| `Grid` | `src/engine/Grid.h` | Fixed 32x64, toroidal wrap, age, double-buffer |
| `GameOfLife` | `src/engine/GameOfLife.h/.cpp` | 5 rule presets, bitmask lookup, xorshift64 PRNG |
| `CellEditQueue` | `src/engine/CellEditQueue.h` | Lock-free SPSC, 256 capacity, cache-line aligned |

## Test Metrics
- Grid tests: 9 (basics, wrapping, age, copy, equality)
- GoL correctness: 10 (Blinker, Block, Beehive, Glider, generation, age, seeding, density)
- Rule variants: 4 (HighLife, Seeds, Ambient, toroidal)
- SPSC queue: 4 (push/pop, full, drain, bounded drain)
- Mutation tests: 3 (birth rule flip, survival disabled, age reset)
- Mutation survival rate: 0% (all 3 mutations caught)
