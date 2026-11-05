cmake_minimum_required(VERSION 3.22)

project(HardKick VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

option(HK_GUI_SNAPSHOT "Build a headless tool that renders the editor to a PNG (for development)" OFF)

# JUCE: use a local checkout if JUCE_DIR is given, otherwise download it.
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

if(NOT HK_GUI_SNAPSHOT)
    juce_add_plugin(HardKick
        COMPANY_NAME "Spencer"
        BUNDLE_ID com.spencer.hardkick
        PLUGIN_MANUFACTURER_CODE Spnc
        PLUGIN_CODE Hdkk
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Instrument Drum
        FORMATS VST3
        PRODUCT_NAME "HardKick")

    target_sources(HardKick PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

    target_include_directories(HardKick PRIVATE Source)

    target_compile_definitions(HardKick PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

    target_link_libraries(HardKick
        PRIVATE
            juce::juce_audio_processors
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
else()
    juce_add_console_app(GuiSnapshot PRODUCT_NAME "GuiSnapshot")
    target_sources(GuiSnapshot PRIVATE
        tests/gui_snapshot.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
    target_include_directories(GuiSnapshot PRIVATE Source)
    target_compile_definitions(GuiSnapshot PRIVATE
        JucePlugin_Name="HardKick"
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0 JUCE_USE_XRENDER=0
        JUCE_PLUGINHOST_LADSPA=0)
    target_link_libraries(GuiSnapshot PRIVATE
        juce::juce_audio_processors
        juce::juce_recommended_config_flags)
endif()
