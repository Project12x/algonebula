# Project State

## Current Phase
Phase 1 — Skeleton + Thread Safety + UI Foundation (`v0.1.0`)

## Build Status
- VST3: Not yet built (scaffolding complete)
- Standalone: Not yet built (scaffolding complete)
- Tests: HeadlessTest stub created

## Key Classes
| Class | File | Status |
|-------|------|--------|
| `AlgoNebulaProcessor` | `src/PluginProcessor.h/.cpp` | Skeleton with APVTS, SmoothedValue, CPU load |
| `AlgoNebulaEditor` | `src/PluginEditor.h/.cpp` | Dark UI with selectors, volume knob, CPU meter |
| `NebulaLookAndFeel` | `src/ui/NebulaLookAndFeel.h/.cpp` | Gradient arc knobs, glow, Inter/JetBrains fonts |
| `NebulaColours` | `src/ui/NebulaColours.h` | Dark palette tokens |

## Test Metrics
- Phase 1: build verification only (no DSP tests yet)
