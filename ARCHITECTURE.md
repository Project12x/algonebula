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

## Thread Model
- **Audio thread**: `processBlock()` — zero allocation, lock-free reads
- **UI thread**: editor painting, parameter changes via APVTS (atomic)
- **Communication**: SPSC queue (UI->audio), atomic reads (audio->UI), double-buffered grid state

## Key Design Decisions
- All buffers pre-allocated in `prepareToPlay()`
- `SmoothedValue` for all continuously-variable params (20ms default)
- CPU load measured per-block as % of audio budget
- APVTS for all parameters — enables host automation and state persistence
- Fonts embedded as BinaryData (no system font dependency)

## Dependencies
| Library | Purpose |
|---------|---------|
| JUCE 8 | Framework, DSP, OpenGL, audio I/O |
| Gin | Plugin utils, 3D camera, oscillators |
| melatonin_blur | GPU-accelerated glow/shadow |
| Signalsmith DSP | Delay, FDN reverb |
| DaisySP | MoogLadder, DSP primitives |
| Airwindows | Tape saturation, effect quality |
