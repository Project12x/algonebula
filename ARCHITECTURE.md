# Architecture

## High-Level Signal Flow
```
CellularEngine (audio thread, no alloc)
    -> ScaleQuantizer (pitch mapping)
    -> VoiceAllocator (priority, stealing, humanization)
    -> SynthVoice[] (PolyBLEP osc + SubOsc + NoiseLayer + AHDSR + SVF)
    -> StereoMixer (per-voice pan, auto-pan, stereo drift)
    -> EffectChain (reorderable: chorus/phaser/flanger/bitcrush/tape/delay/shimmer/limiter)
    -> FreezeProcessor
    -> SafetyProcessor (DC + ultrasonic + brickwall)
    -> DAC
```

## Engine Architecture (Phase 2)
```
UI Thread                Audio Thread              GL/UI Thread
    |                        |                         |
    |  CellEditQueue (SPSC)  |                         |
    |  --push(row,col,st)--> |                         |
    |                        | drainInto(grid)         |
    |                        | engine.step()           |
    |                        | gridSnapshot.copyFrom() |
    |                        |  ----memcpy-----------> |
    |                        |     (double buffer)     |
```

### CellularEngine Interface
Abstract base class for all automata: `step()`, `randomize()`, `clear()`, `getGrid()`.
All implementations must be allocation-free in `step()`.

### Grid
- Fixed-size `uint8_t[32*64]` cells + `uint16_t[32*64]` ages
- Toroidal wrapping via modular arithmetic
- No runtime allocation (max capacity pre-allocated)

### GameOfLife
- Birth/Survival rules encoded as `uint16_t` bitmasks for O(1) lookup
- Scratch grid pre-allocated for double-buffered stepping
- xorshift64 PRNG for deterministic seeding

### CellEditQueue
- Lock-free SPSC ring buffer (256 slots)
- `alignas(64)` atomics to avoid false sharing
- `drainInto()` template for bounded processing in processBlock

## Thread Model
- **Audio thread**: `processBlock()` — zero allocation, lock-free reads
- **UI thread**: editor painting, parameter changes via APVTS (atomic), cell edits via SPSC queue
- **Communication**: SPSC queue (UI->audio), atomic reads (audio->UI), double-buffered grid state

## Key Design Decisions
- All buffers pre-allocated in `prepareToPlay()`
- `SmoothedValue` for all continuously-variable params (20ms default)
- CPU load measured per-block as % of audio budget
- APVTS for all parameters — enables host automation and state persistence
- Fonts embedded as BinaryData (no system font dependency)
- Engine tests are pure C++ (no JUCE) for fast compile-test cycles

## Dependencies
| Library | Purpose |
|---------|---------|
| JUCE 8 | Framework, DSP, OpenGL, audio I/O |
| Gin | Plugin utils, 3D camera, oscillators |
| melatonin_blur | GPU-accelerated glow/shadow |
| Signalsmith DSP | Delay, FDN reverb |
| DaisySP | MoogLadder, DSP primitives |
| Airwindows | Tape saturation, effect quality |
