# How To

## Prerequisites
- Windows 10/11
- Visual Studio 2022 (MSVC C++20)
- CMake 3.22+
- ASIO SDK at `C:\SDKS\asiosdk\ASIOSDK`
- Git

## Build

```bash
# Configure (first time — downloads dependencies via FetchContent)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build VST3 + Standalone
cmake --build build --config Release

# Run headless tests
./build/AlgoNebulaTests.exe

# Run standalone
./build/AlgoNebula_artefacts/Release/Standalone/AlgoNebula.exe
```

## Development

### Adding a new source file
1. Create `.h/.cpp` in the appropriate `src/` subdirectory
2. Add `.cpp` to `target_sources(AlgoNebula ...)` in `CMakeLists.txt`
3. Reconfigure: `cmake -B build`

### Adding a new parameter
1. Add to `createParameterLayout()` in `PluginProcessor.cpp`
2. Add UI control + attachment in `PluginEditor.cpp`
3. Read in `processBlock()` via `apvts.getRawParameterValue()`

### Build preservation
Before building, copy previous artifacts to `releases/YYYY-MM-DD_HHMM/`.

## Common Tasks

### Clean rebuild
```bash
Remove-Item -Recurse -Force build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
