# Algo Nebula

Generative ambient synthesizer plugin (VST3/Standalone) powered by cellular automata and emergent algorithms.

## Features

- **7 CA Engines**: Game of Life, Brian's Brain, Cyclic CA, Reaction-Diffusion, Lenia, Particle Swarm, Brownian Field
- **GPU Compute**: Optional WebGPU acceleration for all engines via ghostsun_render (Dawn)
- **8 Waveshapes**: PolyBLEP anti-aliased oscillators (Sine, Saw, Pulse, Triangle + 4 composites)
- **9 Effects**: Chorus, Delay, Reverb, Phaser, Flanger, Bitcrush, Tape Saturation, Shimmer, Ping Pong
- **Modulation Matrix**: 2 global LFOs with 18 routeable destinations
- **Musical Intelligence**: Note probability, melodic inertia, velocity humanization, density-driven dynamics
- **15 Scales x 12 Keys**: Chromatic through exotic modes, pentatonic, blues, whole-tone
- **3 Tuning Systems**: 12-TET, Just Intonation, Pythagorean with adjustable A4 reference
- **12 Grid Sizes**: From Small (8x12) to Ultra (1280x1280) with engine-aware visualization
- **16 Factory Presets**: Musical, experimental, and utility categories
- **MIDI Keyboard**: Built-in virtual keyboard, MIDI note-on triggers grid reseed

## Quick Start

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Run standalone
./build/AlgoNebula_artefacts/Release/Standalone/Algo\ Nebula.exe

# Run tests
./build/Release/AlgoNebulaTests.exe
```

## Requirements
- CMake 3.22+
- C++20 compiler (MSVC 2022+ / Clang 15+)
- ASIO SDK at `C:\SDKS\asiosdk\ASIOSDK`

## Dependencies (auto-fetched)
- JUCE 8.0.4
- Gin (plugin utils)
- melatonin_blur (glow effects)
- Signalsmith DSP (header-only)
- DaisySP (DSP primitives)
- ghostsun_render (WebGPU/Dawn GPU compute)

## License
Proprietary. All rights reserved.
