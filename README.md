# Algo Nebula

Generative ambient synthesizer plugin (VST3/Standalone) powered by cellular automata and emergent algorithms.

## Features

- **7 CA Engines**: Game of Life, Brian's Brain, Cyclic CA, Reaction-Diffusion, Lenia, Particle Swarm, Brownian Field
- **8 Waveshapes**: PolyBLEP anti-aliased oscillators (Sine, Saw, Pulse, Triangle + 4 composites)
- **Musical Intelligence**: Note probability, melodic inertia, velocity humanization, density-driven dynamics
- **15 Scales x 12 Keys**: Chromatic through exotic modes, pentatonic, blues, whole-tone
- **3 Tuning Systems**: 12-TET, Just Intonation, Pythagorean with adjustable A4 reference
- **Adjustable Grid**: 4 sizes (8x12 to 24x32) with engine-aware visualization
- **11 Factory Presets**: Musical, experimental, and utility categories
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

## License
Proprietary. All rights reserved.
