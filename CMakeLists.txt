cmake_minimum_required(VERSION 3.22)
project(AlgoNebula
    VERSION 0.1.0
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- ASIO SDK ---
set(ASIO_SDK_DIR "C:/SDKS/asiosdk/ASIOSDK" CACHE PATH "Path to Steinberg ASIO SDK")

# --- CPM for dependency management ---
include(cmake/CPM.cmake)

# JUCE 8.0.4
CPMAddPackage(
    NAME JUCE
    GIT_TAG 8.0.4
    GITHUB_REPOSITORY juce-framework/JUCE
    OPTIONS
        "JUCE_BUILD_EXTRAS OFF"
        "JUCE_BUILD_EXAMPLES OFF"
)

# NOTE: Gin deferred to Phase 4+

# melatonin_blur -- download only, add as JUCE module manually
CPMAddPackage(
    NAME melatonin_blur
    GIT_REPOSITORY https://github.com/sudara/melatonin_blur.git
    GIT_TAG main
    DOWNLOAD_ONLY YES
    SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/melatonin_blur"
)
juce_add_module("${CMAKE_BINARY_DIR}/_deps/melatonin_blur")

# Header-only libs
CPMAddPackage(
    NAME signalsmith_dsp
    GIT_REPOSITORY https://github.com/Signalsmith-Audio/dsp.git
    GIT_TAG main
    DOWNLOAD_ONLY YES
)

CPMAddPackage(
    NAME daisysp
    GIT_REPOSITORY https://github.com/electro-smith/DaisySP.git
    GIT_TAG master
    DOWNLOAD_ONLY YES
)

# --- Binary Resources (fonts) ---
juce_add_binary_data(AlgoNebulaResources
    SOURCES
        resources/fonts/Inter-Regular.ttf
        resources/fonts/Inter-Medium.ttf
        resources/fonts/Inter-SemiBold.ttf
        resources/fonts/JetBrainsMono-Regular.ttf
        resources/fonts/JetBrainsMono-Light.ttf
)

# --- Plugin Target ---
juce_add_plugin(AlgoNebula
    COMPANY_NAME                "Antigravity"
    PLUGIN_MANUFACTURER_CODE    AGVY
    PLUGIN_CODE                 ANEB
    FORMATS                     VST3 Standalone
    PRODUCT_NAME                "Algo Nebula"
    BUNDLE_ID                   "com.antigravity.algonebula"
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_SYNTH                    TRUE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
)

# --- Source Files ---
target_sources(AlgoNebula PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/ui/NebulaLookAndFeel.cpp
    src/ui/NebulaColours.cpp
    src/engine/GameOfLife.cpp
    src/engine/BriansBrain.cpp
    src/engine/CyclicCA.cpp
    src/engine/ReactionDiffusion.cpp
    src/engine/LeniaEngine.cpp
    src/engine/ParticleSwarm.cpp
    src/engine/BrownianField.cpp
)

# --- Include Directories ---
target_include_directories(AlgoNebula PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    # signalsmith: use include/ subdir to avoid windows.h shadowing the real Windows SDK header
    ${signalsmith_dsp_SOURCE_DIR}/include
    ${daisysp_SOURCE_DIR}/Source
)

# --- Compile Definitions ---
target_compile_definitions(AlgoNebula PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_ASIO=1
    JUCE_ASIO_DEBUGGING=0
    JUCE_MODAL_LOOPS_PERMITTED=1
    JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=0
    HB_MUTEX_IMPL_STD_MUTEX=1
)

# ASIO headers
if(WIN32 AND EXISTS "${ASIO_SDK_DIR}/common/asio.h")
    message(STATUS "ASIO SDK found at ${ASIO_SDK_DIR}")
    target_include_directories(AlgoNebula PRIVATE "${ASIO_SDK_DIR}/common")
endif()

# MSVC parallel build
if(MSVC)
    target_compile_options(AlgoNebula PRIVATE /FS)
endif()

# --- Link Libraries ---
target_link_libraries(AlgoNebula
    PRIVATE
        AlgoNebulaResources
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_devices
        juce::juce_gui_basics
        juce::juce_dsp
        melatonin_blur
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# --- Headless Test Target ---
# Engine tests are pure C++ (no JUCE dependency) for fast compilation.
add_executable(AlgoNebulaTests
    test/HeadlessTest.cpp
    src/engine/GameOfLife.cpp
    src/engine/BriansBrain.cpp
    src/engine/CyclicCA.cpp
    src/engine/ReactionDiffusion.cpp
    src/engine/LeniaEngine.cpp
    src/engine/ParticleSwarm.cpp
    src/engine/BrownianField.cpp
)

target_include_directories(AlgoNebulaTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(AlgoNebulaTests PRIVATE cxx_std_17)

