cmake_minimum_required(VERSION 3.22)
project(DemarcoTone VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# JUCE
# Reuse the JUCE checkout from the sibling pedal-steel-vst project.
# If you move this project, replace this with a direct submodule:
#     add_subdirectory(JUCE)
# ---------------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../pedal-steel-vst/JUCE/CMakeLists.txt")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../pedal-steel-vst/JUCE" JUCE)
else()
    message(FATAL_ERROR "JUCE not found. Clone JUCE into ./JUCE or a sibling project.")
endif()

juce_add_plugin(DemarcoTone
    COMPANY_NAME "jimsvoid"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Jmsv
    PLUGIN_CODE BgDw
    # VST3 is fine provided the project is built from a path without
    # apostrophes. Access this project via the ~/musica-drive symlink, not
    # /Volumes/Jim's Extra/ directly — JUCE's VST3 manifest-helper shells
    # out without escaping single quotes and will fail.
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "BigDawg"
    BUNDLE_ID "com.jimsvoid.bigdawg"
    COPY_PLUGIN_AFTER_BUILD TRUE
)

juce_generate_juce_header(DemarcoTone)

target_sources(DemarcoTone
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/DSP/SoftClip.cpp
        Source/DSP/SoftClip.h
        Source/DSP/Detune.cpp
        Source/DSP/Detune.h
        Source/DSP/Chorus.cpp
        Source/DSP/Chorus.h
        Source/DSP/Vibrato.cpp
        Source/DSP/Vibrato.h
        Source/DSP/WowFlutter.cpp
        Source/DSP/WowFlutter.h
        Source/DSP/SpringReverb.cpp
        Source/DSP/SpringReverb.h
        Source/DSP/Bitcrusher.cpp
        Source/DSP/Bitcrusher.h
        Source/DSP/SlapDelay.cpp
        Source/DSP/SlapDelay.h
        Source/DSP/PlateReverb.cpp
        Source/DSP/PlateReverb.h
)

target_include_directories(DemarcoTone
    PRIVATE
        Source
)

target_compile_definitions(DemarcoTone
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(DemarcoTone
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
