cmake_minimum_required(VERSION 3.22)

project(FLstudioIR_Television VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(FLstudioIR_Television
    COMPANY_NAME "FLstudioIR"
    PLUGIN_NAME "Television"
    IS_SYNTH FALSE
    IS_MIDI_EFFECT FALSE
    PLUGIN_MANUFACTURER_CODE "Flir"
    PLUGIN_CODE_ "Tv90"
    FORMATS VST3 AU CLAP Standalone
    PRODUCT_NAME "FLstudioIR Television"
)

juce_generate_juce_header(FLstudioIR_Television)

target_sources(FLstudioIR_Television PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_link_libraries(FLstudioIR_Television PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_cryptography
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

target_compile_definitions(FLstudioIR_Television PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
