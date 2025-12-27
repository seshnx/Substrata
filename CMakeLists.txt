cmake_minimum_required(VERSION 3.22)
project(Substrata VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# AAX SDK Path
set(AAX_SDK_PATH "C:/aax-sdk-2-9-0" CACHE PATH "Path to AAX SDK")

add_subdirectory(JUCE)

# Set AAX SDK path
juce_set_aax_sdk_path(${AAX_SDK_PATH})

# Binary resources (for logo and other assets)
juce_add_binary_data(Substrata_BinaryData
    SOURCES
        Resources/company_logo.png
    HEADER_NAME BinaryData.h
)

juce_add_plugin(Substrata
    COMPANY_NAME "Amalia Media LLC"
    PLUGIN_MANUFACTURER_CODE Amal
    PLUGIN_CODE Sesh
    FORMATS VST3 AU Standalone AAX
    PRODUCT_NAME "SeshNx Substrata"
    DESCRIPTION "Harmonic Shifter with Microtonal Control"
    VERSION "1.0.0"
)

target_sources(Substrata PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PitchTracker.cpp
    Source/PitchTracker.h
    Source/PitchShifter.cpp
    Source/PitchShifter.h
    Source/HarmonicGenerator.cpp
    Source/HarmonicGenerator.h
    Source/ScaleDatabase.cpp
    Source/ScaleDatabase.h
)

target_compile_definitions(Substrata
    PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Substrata
    PRIVATE
    Substrata_BinaryData
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

