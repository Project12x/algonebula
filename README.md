# Algo Nebula

Generative ambient synthesizer plugin (VST3/Standalone) powered by cellular automata and emergent algorithms.

## Quick Start

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Run standalone
./build/AlgoNebula_artefacts/Release/Standalone/AlgoNebula.exe
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
