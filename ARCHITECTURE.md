# Architecture

## High-Level Signal Flow
```
CellularEngine (7 engines, CPU path, message thread via CpuStepTimer)
    OR
GpuComputeManager (7 GPU adapters, message thread, WebGPU/Dawn)
    -> GpuGridBridge (lock-free float double-buffer)
    -> processBlock reads floats via readLock/readUnlock
    -> ScaleQuantizer (pitch mapping, 15 scales x 12 keys)
    -> Musicality Layer (noteProbability, melodicInertia, velocityHumanize)
    -> SynthVoice[] (PolyBLEP osc + SubOsc + NoiseLayer + AHDSR + SVF)
    -> Density Modulation (grid density -> gain + filter cutoff)
    -> StereoMixer (per-voice pan from grid column position)
    -> EffectChain (9 effects, parallel routing)
    -> SafetyProcessor (DC block + brickwall limiter)
    -> DAC
```

## Grid-to-Music Mapping

On each clock step (driven by BPM + division), `processBlock` scans the grid for newly born cells and maps them to synth voices:

```
1. BIRTH DETECTION (per clock step)
   For each cell in the grid:
     - CPU: grid.wasBorn(r, c) checks birth flag
     - GPU: GpuGridBridge::wasBornLocked(current, previous, r, c)
       (current > 0.5 AND previous <= 0.5, i.e., cell just appeared)
   
2. TRIGGER BUDGET
   Each engine defines a max voices-per-step (triggerBudget):
     GoL=8, Brian's Brain=6, Cyclic=5, R-D=4, Lenia=4,
     ParticleSwarm=4, BrownianField=6
   Only the first N newborn cells fire notes.

3. PITCH: cell position -> MIDI note
   ScaleQuantizer::quantize(row, col, rows, cols, ...)
     - Row maps to pitch range (higher rows = higher notes)
     - Constrained to the selected scale (15 scales x 12 root keys)
     - Optional Pitch Gravity: bias toward melodic neighbors
     - Optional Consonance Filter: snap dissonant intervals

4. VELOCITY: cell intensity -> loudness
   - GpuGridBridge::getCellIntensityLocked() reads float value
   - Multiplied by optional velocity humanization (random +/- offset)
   - Multiplied by engine gain scale (GoL=1.0, R-D=0.4, Lenia=0.4, etc.)

5. STEREO: column position -> pan
   - pan = (2 * col / (cols-1) - 1) * stereoWidth
   - Left edge = full left, right edge = full right

6. VOICE ALLOCATION
   - 64-voice pool with round-robin or lowest-envelope stealing
   - Each voice: PolyBLEP oscillator + sub + noise + AHDSR + SVF filter
   - Gate time: auto-release after fractional step interval
   - Strum spread: onset delay per column position
```

The visualization IS the music: cells appearing in different rows produce
different pitches, and their column determines stereo position.

## GPU / CPU Dual Path

```
GPU path (gpuAccel=ON):
  Message Thread              Audio Thread
      |                           |
  GpuComputeManager               |
  timerCallback() ~60Hz          |
      |                           |
  ComputeSimulation::step()      |
  GPU readback -> float[]        |
      |                           |
  GpuGridBridge::updateFromGpu() |
  (atomic pointer swap)          |
      |                           |
      |  ---readLock()---------> |
      |     float* current       |
      |     float* previous      |
      |  ---readUnlock()-------> |
      |                           |
      |  convertToGrid()         |
      |  (generation-gated)      |
      |     -> gridSnapshot      |

CPU path (gpuAccel=OFF):
  Message Thread              Audio Thread
      |                           |
  CpuStepTimer ~60Hz              |
      |                           |
  engine->step()                  |
  bridge.updateFromCpu()          |
  (atomic flag consumed)          |
      |                           |
      |  ---readLock()----------> |
      |     float* current        |
      |     float* previous       |
      |  ---readUnlock()--------> |
      |                           |
      |  convertToGrid()          |
      |  (generation-gated)       |
      |     -> gridSnapshots_     |
```

Both paths share the same architecture: simulation on message thread,
audio thread reads only from GpuGridBridge (lock-free).

### GpuGridBridge
- Float-only double-buffer with atomic pointer swap
- `updateFromGpu(float*, size_t)`: writer side (message thread)
- `updateFromCpu(Grid&)`: writer side (audio thread, CPU path)
- `readLock/readUnlock`: consumer side (audio thread)
- `convertToGrid(Grid&)`: generation-gated conversion to Grid for UI painting
  - Stores float intensity as age (0-255) for continuous engine rendering
  - Only runs when bridge generation changes (prevents 1.6M ops/sec at large grids)
- Previous buffer snapshot for birth detection (`wasBornLocked`)

## Engine Architecture
```
UI Thread                Message Thread            Audio Thread
    |                        |                         |
    |  CellEditQueue (SPSC)  |                         |
    |  --push(row,col,st)--> |                         |
    |                        |                         |
    |                   CpuStepTimer ~60Hz              |
    |                        |                         |
    |                   drainInto(grid)                 |
    |                   engine->step()                  |
    |                   bridge.updateFromCpu()          |
    |                        |                         |
    |                        |  audio reads bridge      |
    |                        |  (voice triggering)      |
    |                        |                         |
    |                        |  convertToGrid()         |
    |                        |   -> gridSnapshots_ ---> | GridComponent::paint()
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
- Dynamic size: 12 options from Small (8x12) to Ultra (1280x1280)
- Max capacity: 1280x1280 cells + ages (std::vector heap storage)
- `wasBorn()` tracking for event-based note triggering
- Toroidal wrapping via modular arithmetic
- Age field used for both GoL age tracking and float intensity storage

### CellEditQueue
- Lock-free SPSC ring buffer (256 slots)
- `alignas(64)` atomics to avoid false sharing
- `drainInto()` template for bounded processing in processBlock

## Musicality System

### Note Triggering
Voice triggering occurs on cell birth events. Three musicality parameters modulate this:

- **noteProbability** (0-1): Random gate -- each birth rolls against this threshold
- **melodicInertia** (0-1): Probability of reusing last triggered pitch
- **velocityHumanize** (0-0.3): Random velocity offset

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
- O(1) array lookup -- pre-computed degree tables

### Microtuning
- 3 tuning systems: 12-TET (equal), Just Intonation (5-limit), Pythagorean (3:2 stacking)
- Adjustable A4 reference (default 440Hz)
- Pre-computed 128-note frequency table -- O(1) RT lookup

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
- Max 64 voices with quietest-voice stealing
- Stereo panning from grid column position (equal-power)
- Grid position tracked per-voice for cell-death release

## Effects Chain

### StereoEffect Base
- Abstract interface: `init()`, `process()`, `reset()`, `getMix()`, `setMix()`, `isBypassed()`
- 9 effects: StereoChorus, StereoDelay, PlateReverb, StereoPhaser, StereoFlanger, Bitcrush, TapeSaturation, ShimmerReverb, PingPongDelay
- APVTS toggle per effect for quick A/B switching
- Parallel send/return routing (prevents cascading feedback)

### SafetyProcessor
- DC block (5Hz HPF) + ultrasonic filter (20kHz LPF) + brickwall limiter (-0.3dBFS)
- Always active, never bypassed

### Modulation Matrix
- 2 global LFOs (sine/tri/saw/square/S&H, 0.01-20Hz)
- Bipolar amount per destination
- 18 routeable destinations (effect mixes, rates, filter, envelope)

## Visualization

### GridComponent (Engine-Aware)
Each engine renders with a distinct color palette:
- **GoL**: Birth/alive age gradient from `NebulaColours`
- **Brian's Brain**: Cyan (alive) / Amber (dying) 3-state
- **Cyclic CA**: HSL hue wheel mapped from cell state
- **R-D / Lenia / Swarm / Brownian**: Intensity heatmaps from float data (mapped via age field)

Status bar shows engine name + generation counter.

## Thread Model
- **Audio thread**: `processBlock()` — zero allocation, lock-free reads from bridge, sets atomic step/reseed/clear flags
- **Message thread**: `GpuComputeManager::timerCallback()` (GPU), `CpuStepTimer::timerCallback()` (CPU) — engine stepping, cell edit drain, editor painting
- **UI thread**: parameter changes via APVTS (atomic), cell edits via SPSC queue
- **Communication**: SPSC queue (UI->message thread), atomic flags (audio->message thread), GpuGridBridge (message->audio), double-buffered grid snapshots (audio->UI)

## Key Design Decisions
- All buffers pre-allocated in `prepareToPlay()`
- APVTS for all parameters -- enables host automation and state persistence
- GPU is opt-in (`gpuAccel` default OFF) for iGPU/low-end compatibility
- Fonts embedded as BinaryData (no system font dependency)
- Engine tests are pure C++ (no JUCE) for fast compile-test cycles
- Factory method pattern for engine creation -- no runtime type switching in audio path
- xorshift64 PRNG for all randomization -- deterministic, allocation-free
- Generation-gated grid conversion -- prevents unnecessary work on audio thread

## Dependencies
| Library | Purpose |
|---------|---------|
| JUCE 8 | Framework, DSP, audio I/O |
| Gin | Plugin utils |
| melatonin_blur | GPU-accelerated glow/shadow |
| Signalsmith DSP | Delay, FDN reverb |
| DaisySP | DSP primitives |
| ghostsun_render | WebGPU/Dawn GPU compute + rendering |
