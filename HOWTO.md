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

# Build VST3 + Standalone + Tests
cmake --build build --config Release

# Run headless tests
./build/Release/AlgoNebulaTests.exe

# Run standalone
./build/AlgoNebula_artefacts/Release/Standalone/Algo\ Nebula.exe
```

## Development

### Adding a new source file
1. Create `.h/.cpp` in the appropriate `src/` subdirectory
2. Add `.cpp` to `target_sources(AlgoNebula ...)` in `CMakeLists.txt`
3. Reconfigure: `cmake -B build`

### Adding a new parameter
1. Add to `createParameterLayout()` in `PluginProcessor.cpp`
2. Add UI control + attachment in `PluginEditor.h/.cpp`
3. Read in `processBlock()` via `apvts.getRawParameterValue()`
4. Add to factory presets in `FactoryPresets.h`

### Adding a new CA engine
1. Create engine class in `src/engine/` inheriting `CellularEngine`
2. Implement: `step()`, `randomize()`, `randomizeSymmetric()`, `clear()`, `getGrid()`, `getType()`, `getName()`
3. Add `EngineType` enum value in `CellularEngine.h`
4. Add factory case in `PluginProcessor::createEngine()`
5. Add visualization case in `GridComponent::paint()` if engine has native data
6. Add tests in `tests/main.cpp`

### Adding a factory preset
1. Edit `src/engine/FactoryPresets.h`
2. Add `FactoryPreset` struct to the vector in `getFactoryPresets()`
3. Set all parameter values (check `createParameterLayout()` for valid ranges)
4. Verify algorithm index matches `EngineType` enum (0=GoL, 1=BriansBrain, etc.)

### Build preservation
Before building, copy previous artifacts to `releases/YYYY-MM-DD_HHMM/`.

## Common Tasks

### Clean rebuild
```bash
Remove-Item -Recurse -Force build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Run tests only
```bash
cmake --build build --config Release --target AlgoNebulaTests
./build/Release/AlgoNebulaTests.exe
```
