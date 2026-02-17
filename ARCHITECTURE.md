# Architecture

## High-Level Signal Flow
```
CellularEngine (7 engines, audio thread, no alloc)
    -> ScaleQuantizer (pitch mapping, 15 scales x 12 keys)
    -> Musicality Layer (noteProbability, melodicInertia, velocityHumanize)
    -> SynthVoice[] (PolyBLEP osc + SubOsc + NoiseLayer + AHDSR + SVF)
    -> Density Modulation (grid density -> gain + filter cutoff)
    -> StereoMixer (per-voice pan from grid column position)
    -> [Future: EffectChain, FreezeProcessor, SafetyProcessor]
    -> DAC
```

## Engine Architecture
```
UI Thread                Audio Thread              GL/UI Thread
    |                        |                         |
    |  CellEditQueue (SPSC)  |                         |
    |  --push(row,col,st)--> |                         |
    |                        | drainInto(grid)         |
    |                        | engine->step()          |
    |                        | gridSnapshot.copyFrom() |
    |                        |  ----memcpy-----------> |
    |                        |     (double buffer)     |
```

### CellularEngine Interface
Abstract base class for all automata: `step()`, `randomize()`, `randomizeSymmetric()`, `clear()`, `getGrid()`, `getType()`, `getName()`.
All implementations allocation-free in `step()`. Factory method in processor creates engine by `EngineType` enum.

### Engine Implementations

| Engine | Type | Data Model | Musical Character |
|--------|------|------------|-------------------|
| GameOfLife | Binary | `uint8_t` grid | Rhythmic, structured |
| BriansBrain | 3-state | `uint8_t` (0/1/2) | Sparking, chaotic |
| CyclicCA | N-state | `uint8_t` (0-15) | Rotating spirals |
| ReactionDiffusion | Continuous | `float` chemical fields | Evolving textures |
| LeniaEngine | Continuous | `float` state field (0-1) | Organic, morphing blobs |
| ParticleSwarm | Agent-based | Particle positions + `float` trail field | Flocking patterns |
| BrownianField | Continuous | Walker positions + `float` energy field | Random walk deposits |

### Grid
- Dynamic size: Small (8x12), Medium (12x16), Large (16x24), XL (24x32)
- Max capacity: `uint8_t[32*64]` cells + `uint16_t[32*64]` ages
- `wasBorn()` tracking for event-based note triggering
- Toroidal wrapping via modular arithmetic
- No runtime allocation

### CellEditQueue
- Lock-free SPSC ring buffer (256 slots)
- `alignas(64)` atomics to avoid false sharing
- `drainInto()` template for bounded processing in processBlock

## Musicality System

### Note Triggering
Voice triggering occurs on cell birth events (`wasBorn()`). Three musicality parameters modulate this:

- **noteProbability** (0-1): Random gate — each birth rolls against this threshold; miss = skip
- **melodicInertia** (0-1): Probability of reusing last triggered pitch vs computing new from grid position
- **velocityHumanize** (0-0.3): Random velocity offset applied to each triggered note

### Density-Driven Dynamics
Grid density (alive cells / total cells) modulates:
- **Voice gain**: Dense grids = softer (1.0 at 0% density, 0.35 at 100%)
- **Filter cutoff**: Dense grids = brighter (0.5x at 0%, 1.0x at 100%)

### Voice Lifecycle
- Voices track their grid position (`gridRow`, `gridCol`)
- Cell death releases the associated voice via `noteOff()`
- Voice stealing: quietest voice stolen when all slots full
- One note per column per step (prevents chords from single step)

## Clock + Music Theory

### ClockDivider
- Integer sample counting (no floating-point drift)
- 6 divisions: whole, half, quarter, eighth, sixteenth, thirty-second
- Swing timing: 50% (straight) to 75% (max swing)
- Drives `engine->step()` from `processBlock()`

### ScaleQuantizer
- 15 scales: Chromatic, Major/Minor, 7 modes, 2 pentatonics, Blues, Whole-Tone, Harmonic/Melodic Minor
- 12 root keys (0=C through 11=B)
- O(1) array lookup — pre-computed degree tables

### Microtuning
- 3 tuning systems: 12-TET (equal), Just Intonation (5-limit), Pythagorean (3:2 stacking)
- Adjustable A4 reference (default 440Hz)
- Pre-computed 128-note frequency table — O(1) RT lookup

## Synth Voices

### PolyBLEPOscillator
- 8 waveshapes: Sine, Triangle, Saw, Pulse + 4 composite (SineOct, FifthStack, Pad, Bell)
- Polynomial bandlimited step (PolyBLEP) at discontinuities for anti-aliasing
- Pulse width modulation, per-column waveshape cycling

### AHDSREnvelope
- 5-stage envelope: Attack -> Hold -> Decay -> Sustain -> Release
- Linear ramps, retrigger from current level (no click artifacts)

### SVFilter
- State-variable filter (Cytomic/Simper topology)
- 4 modes: Low Pass, High Pass, Band Pass, Notch

### SynthVoice
- Composite: PolyBLEPOscillator + SubOscillator + NoiseLayer + AHDSREnvelope + SVFilter
- Max 8 voices with quietest-voice stealing
- Stereo panning from grid column position (equal-power)
- Grid position tracked per-voice for cell-death release

## Visualization

### GridComponent (Engine-Aware)
Each engine renders with a distinct color palette:
- **GoL**: Birth/alive intensity from `NebulaColours`
- **Brian's Brain**: Cyan (alive) / Amber (dying) 3-state
- **Cyclic CA**: HSL hue wheel mapped from cell state
- **R-D / Lenia / Swarm / Brownian**: Intensity heatmaps from native float fields

Status bar shows engine name + generation counter.

## Thread Model
- **Audio thread**: `processBlock()` — zero allocation, lock-free reads
- **UI thread**: editor painting, parameter changes via APVTS (atomic), cell edits via SPSC queue
- **Communication**: SPSC queue (UI->audio), atomic reads (audio->UI), double-buffered grid state

## Key Design Decisions
- All buffers pre-allocated in `prepareToPlay()`
- APVTS for all parameters — enables host automation and state persistence
- Fonts embedded as BinaryData (no system font dependency)
- Engine tests are pure C++ (no JUCE) for fast compile-test cycles
- Factory method pattern for engine creation — no runtime type switching in audio path
- xorshift64 PRNG for all randomization (musicality + seeding) — deterministic, allocation-free

## Dependencies
| Library | Purpose |
|---------|---------|
| JUCE 8 | Framework, DSP, audio I/O |
| Gin | Plugin utils |
| melatonin_blur | GPU-accelerated glow/shadow |
| Signalsmith DSP | Delay, FDN reverb (future) |
| DaisySP | DSP primitives (future) |
