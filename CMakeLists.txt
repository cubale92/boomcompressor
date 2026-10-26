cmake_minimum_required(VERSION 3.22)
project(Boom8 VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: uses a local copy if JUCE_DIR is given, otherwise downloads it.
set(JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (optional)")
if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Boom8
    COMPANY_NAME "8cee"
    BUNDLE_ID "com.8cee.boom8"
    PLUGIN_MANUFACTURER_CODE Cee8
    PLUGIN_CODE Bm8c
    PRODUCT_NAME "8cee Boom-8"
    FORMATS VST3 AU Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Dynamics
    AU_MAIN_TYPE kAudioUnitType_Effect)

target_sources(Boom8 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Boom8 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Boom8
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Offline tests (not needed for normal builds)
option(BOOM8_TESTS "Build offline DSP tests" OFF)
if(BOOM8_TESTS)
    add_executable(Boom8Tests Tests/TestMain.cpp)
    target_link_libraries(Boom8Tests PRIVATE Boom8)
    target_include_directories(Boom8Tests PRIVATE $<TARGET_PROPERTY:Boom8,INCLUDE_DIRECTORIES>)
    target_compile_definitions(Boom8Tests PRIVATE $<TARGET_PROPERTY:Boom8,COMPILE_DEFINITIONS>)
endif()
