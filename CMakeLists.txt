cmake_minimum_required(VERSION 3.22)

# macOS: universal binary (Apple Silicon + Intel), set before project()
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "" FORCE)

project(WaveFoldingBoss VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(WFB_COPY_AFTER_BUILD "Copy plugins to the system plugin folders after building" OFF)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

set(WFB_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND WFB_FORMATS AU)
endif()

juce_add_plugin(WaveFoldingBoss
    COMPANY_NAME "Pink Leopard Audio"
    PRODUCT_NAME "Wave Folding Boss"
    PLUGIN_MANUFACTURER_CODE Plea
    PLUGIN_CODE Wfbs
    FORMATS ${WFB_FORMATS}
    VST3_CATEGORIES Fx Distortion
    AU_MAIN_TYPE kAudioUnitType_Effect
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD ${WFB_COPY_AFTER_BUILD})

target_sources(WaveFoldingBoss PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(WaveFoldingBoss PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(WaveFoldingBoss
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
