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

## Clock + Music Theory (Phase 3)

### ClockDivider
- Integer sample counting (no floating-point drift)
- 6 divisions: whole, half, quarter, eighth, sixteenth, thirty-second
- Swing timing: 50% (straight) to 75% (max swing)
- Drives `engine.step()` from `processBlock()`

### ScaleQuantizer
- 15 scales: Chromatic, Major/Minor, 7 modes, 2 pentatonics, Blues, Whole-Tone, Harmonic/Melodic Minor
- 12 root keys (0=C through 11=B)
- O(1) array lookup — pre-computed degree tables

### Microtuning
- 3 tuning systems: 12-TET (equal), Just Intonation (5-limit), Pythagorean (3:2 stacking)
- Adjustable A4 reference (default 440Hz)
- Pre-computed 128-note frequency table — O(1) RT lookup

## Synth Voices (Phase 4)

### PolyBLEPOscillator
- 8 waveshapes: Sine, Triangle, Saw, Pulse + 4 composite (SineOct, FifthStack, Pad, Bell)
- Polynomial bandlimited step (PolyBLEP) at discontinuities for anti-aliasing
- Pulse width modulation control

### AHDSREnvelope
- 5-stage envelope: Attack -> Hold -> Decay -> Sustain -> Release
- Linear ramps, retrigger from current level (no click artifacts)
- Per-sample increment pre-computation for RT-safety

### SVFilter
- State-variable filter (Cytomic/Simper topology)
- 4 modes: Low Pass, High Pass, Band Pass, Notch
- Coefficients recalculated only on parameter change

### SynthVoice
- Composite: PolyBLEPOscillator + SubOscillator + NoiseLayer + AHDSREnvelope + SVFilter
- 8-voice polyphony with quietest-voice stealing
- Grid cells -> ScaleQuantizer -> Microtuning -> frequency
- Stereo panning based on grid column position

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
