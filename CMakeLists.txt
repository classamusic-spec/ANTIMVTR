cmake_minimum_required(VERSION 3.22)

project(AntiMatr
        VERSION 0.1.0
        DESCRIPTION "ANTI-MATR — Sound Beyond Matter"
        LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# Global configuration
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

option(ANTIMATR_DEV                "Enable developer diagnostics + DSP LAB"       ON)
option(ANTIMATR_BUILD_TESTS        "Build the unit test runner"                    ON)
option(ANTIMATR_BUILD_TOOLS        "Build the offline render + UI snapshot tools"  ON)
option(ANTIMATR_WARNINGS_AS_ERRORS "Treat warnings as errors"                      OFF)
option(ANTIMATR_USE_CCACHE         "Use ccache when available"                     ON)
option(ANTIMATR_COPY_PLUGIN        "Copy plugin to the user plugin folder"         OFF)

if (ANTIMATR_USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if (CCACHE_PROGRAM)
        message(STATUS "ANTI-MATR: using ccache (${CCACHE_PROGRAM})")
        set(CMAKE_C_COMPILER_LAUNCHER   "${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    endif()
endif()

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version" FORCE)
endif()

include(cmake/JUCE.cmake)
include(cmake/Warnings.cmake)

# ---------------------------------------------------------------------------
# Source groups
#   CORE  — host-agnostic engine, state, diagnostics data (no GUI modules)
#   UI    — editor, components, visualizers, DSP LAB
#   PLUGIN— AudioProcessor / editor glue
# ---------------------------------------------------------------------------
file(GLOB_RECURSE ANTIMATR_CORE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/state/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dsp/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/presets/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dev/diagnostics/*.cpp)

file(GLOB_RECURSE ANTIMATR_UI_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dev/dsplab/*.cpp)

file(GLOB ANTIMATR_PLUGIN_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/*.cpp)

# Headers are listed so IDEs show them; they do not affect the build.
file(GLOB_RECURSE ANTIMATR_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

set(ANTIMATR_COMMON_DEFINES
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_USE_WIN_WEBVIEW2=0
    ANTIMATR_VERSION_STRING="${PROJECT_VERSION}"
    ANTIMATR_DEV=$<BOOL:${ANTIMATR_DEV}>)

set(ANTIMATR_CORE_MODULES
    juce::juce_core
    juce::juce_events
    juce::juce_data_structures
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_dsp)

set(ANTIMATR_UI_MODULES
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_processors
    juce::juce_audio_devices
    juce::juce_audio_utils)

# ---------------------------------------------------------------------------
# Optional embedded assets (fonts under assets/fonts, OFL licensed). The
# instrument builds and runs without them; when present they are compiled in
# as BinaryData and the theme picks them up (ANTIMATR_HAS_FONTS=1).
# ---------------------------------------------------------------------------
file(GLOB ANTIMATR_FONT_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/*.ttf ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/*.otf)
set(ANTIMATR_ASSET_TARGETS "")
set(ANTIMATR_ASSET_DEFINES ANTIMATR_HAS_FONTS=0)
if (ANTIMATR_FONT_FILES)
    juce_add_binary_data(AntiMatrAssets HEADER_NAME AntiMatrAssets.h NAMESPACE AntiMatrAssets SOURCES ${ANTIMATR_FONT_FILES})
    set(ANTIMATR_ASSET_TARGETS AntiMatrAssets)
    set(ANTIMATR_ASSET_DEFINES ANTIMATR_HAS_FONTS=1)
    message(STATUS "ANTI-MATR: embedding fonts: ${ANTIMATR_FONT_FILES}")
endif()

# ---------------------------------------------------------------------------
# Plugin
# ---------------------------------------------------------------------------
set(ANTIMATR_FORMATS VST3 Standalone)
if (APPLE)
    list(APPEND ANTIMATR_FORMATS AU)
endif()

juce_add_plugin(AntiMatr
    PRODUCT_NAME                "ANTI-MATR"
    COMPANY_NAME                "Instruments For A More Strange Tomorrow"
    COMPANY_WEBSITE             "https://antimatr.audio"
    BUNDLE_ID                   "com.classamusic.antimatr"
    PLUGIN_MANUFACTURER_CODE    Clsm
    PLUGIN_CODE                 Amtr
    FORMATS                     ${ANTIMATR_FORMATS}
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD     ${ANTIMATR_COPY_PLUGIN}
    VST3_CATEGORIES             Instrument Synth
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    MICROPHONE_PERMISSION_ENABLED FALSE)

target_sources(AntiMatr PRIVATE
    ${ANTIMATR_CORE_SOURCES}
    ${ANTIMATR_UI_SOURCES}
    ${ANTIMATR_PLUGIN_SOURCES}
    ${ANTIMATR_HEADERS})

target_include_directories(AntiMatr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(AntiMatr PUBLIC
    ${ANTIMATR_COMMON_DEFINES}
    ${ANTIMATR_ASSET_DEFINES}
    $<$<PLATFORM_ID:Linux>:JUCE_JACK=1>
    $<$<PLATFORM_ID:Linux>:JUCE_ALSA=1>)

target_link_libraries(AntiMatr
    PRIVATE
        ${ANTIMATR_CORE_MODULES}
        ${ANTIMATR_UI_MODULES}
        ${ANTIMATR_ASSET_TARGETS}
        antimatr_warnings
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)

# ---------------------------------------------------------------------------
# Tests (console, core only — no GUI modules)
# ---------------------------------------------------------------------------
if (ANTIMATR_BUILD_TESTS)
    enable_testing()
    file(GLOB_RECURSE ANTIMATR_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)

    juce_add_console_app(AntiMatrTests PRODUCT_NAME "AntiMatrTests")
    target_sources(AntiMatrTests PRIVATE ${ANTIMATR_CORE_SOURCES} ${ANTIMATR_TEST_SOURCES})
    target_include_directories(AntiMatrTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    target_compile_definitions(AntiMatrTests PRIVATE ${ANTIMATR_COMMON_DEFINES} ANTIMATR_TESTING=1)
    target_link_libraries(AntiMatrTests PRIVATE ${ANTIMATR_CORE_MODULES} antimatr_warnings
        juce::juce_recommended_config_flags)

    add_test(NAME AntiMatrTests COMMAND AntiMatrTests)
endif()

# ---------------------------------------------------------------------------
# Tools
#   AntiMatrRender   — offline renderer: MIDI → WAV + JSON analysis (headless)
#   AntiMatrSnapshot — renders the editor to PNG for visual review (needs X11)
# ---------------------------------------------------------------------------
if (ANTIMATR_BUILD_TOOLS)
    juce_add_console_app(AntiMatrRender PRODUCT_NAME "AntiMatrRender")
    file(GLOB ANTIMATR_RENDER_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/render/*.cpp)
    target_sources(AntiMatrRender PRIVATE ${ANTIMATR_CORE_SOURCES} ${ANTIMATR_RENDER_SOURCES})
    target_include_directories(AntiMatrRender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(AntiMatrRender PRIVATE ${ANTIMATR_COMMON_DEFINES})
    target_link_libraries(AntiMatrRender PRIVATE ${ANTIMATR_CORE_MODULES} antimatr_warnings
        juce::juce_recommended_config_flags)

    juce_add_gui_app(AntiMatrSnapshot PRODUCT_NAME "AntiMatrSnapshot")
    file(GLOB ANTIMATR_SNAPSHOT_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/snapshot/*.cpp)
    # The snapshot tool hosts the real processor + editor, minus the plugin entry point.
    set(ANTIMATR_SNAPSHOT_PLUGIN_SOURCES ${ANTIMATR_PLUGIN_SOURCES})
    list(FILTER ANTIMATR_SNAPSHOT_PLUGIN_SOURCES EXCLUDE REGEX ".*PluginMain\\.cpp$")
    target_sources(AntiMatrSnapshot PRIVATE
        ${ANTIMATR_CORE_SOURCES} ${ANTIMATR_UI_SOURCES} ${ANTIMATR_SNAPSHOT_PLUGIN_SOURCES} ${ANTIMATR_SNAPSHOT_SOURCES})
    target_include_directories(AntiMatrSnapshot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(AntiMatrSnapshot PRIVATE ${ANTIMATR_COMMON_DEFINES} ${ANTIMATR_ASSET_DEFINES} ANTIMATR_SNAPSHOT_TOOL=1
        JUCE_JACK=0 JUCE_ALSA=0)
    target_link_libraries(AntiMatrSnapshot PRIVATE ${ANTIMATR_CORE_MODULES} ${ANTIMATR_UI_MODULES} ${ANTIMATR_ASSET_TARGETS} antimatr_warnings
        juce::juce_recommended_config_flags)
endif()
