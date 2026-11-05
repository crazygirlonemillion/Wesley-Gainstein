cmake_minimum_required(VERSION 3.24)
project(hly_chorfeed VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(HLY_BUILD_PLUGIN "Build the JUCE plugin" ON)
option(HLY_BUILD_TESTS "Build DSP and processor tests" ON)
option(HLY_BUILD_TOOLS "Build offline renderer and UI capture tool" OFF)

include(CTest)
if(HLY_BUILD_TESTS)
    add_executable(hly_dsp_tests tests/dsp_tests.cpp)
    target_include_directories(hly_dsp_tests PRIVATE Source)
    add_test(NAME dsp COMMAND hly_dsp_tests)
endif()
if(HLY_BUILD_TOOLS)
    add_executable(hly_render_demo tests/render_demo.cpp)
endif()

if(HLY_BUILD_PLUGIN)
    include(FetchContent)
    FetchContent_Declare(JUCE
        URL https://github.com/juce-framework/JUCE/archive/refs/tags/8.0.6.tar.gz
        URL_HASH SHA256=95752f8e8b37d802b4c10182bc757de7d88f0c8899e879afa8798589be306a11
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(JUCE)
    set(HLY_FORMATS VST3)
    if(APPLE)
        list(APPEND HLY_FORMATS AU)
    endif()
    juce_add_plugin(HlyChorfeed
        COMPANY_NAME "hly"
        BUNDLE_ID "audio.hly.chorfeed"
        PLUGIN_MANUFACTURER_CODE Hlyy
        PLUGIN_CODE Hlcf
        FORMATS ${HLY_FORMATS}
        PRODUCT_NAME "hly_chorfeed"
        DESCRIPTION "Stereo chorus and feedback network"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Fx Delay Modulation Distortion
        AU_MAIN_TYPE kAudioUnitType_Effect)
    juce_generate_juce_header(HlyChorfeed)
    target_sources(HlyChorfeed PRIVATE
        Source/PluginProcessor.cpp Source/PluginProcessor.h
        Source/PluginEditor.cpp Source/PluginEditor.h Source/DspEngine.h)
    target_compile_definitions(HlyChorfeed PUBLIC
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_REPORT_APP_USAGE=0)
    target_link_libraries(HlyChorfeed PRIVATE
        juce::juce_audio_processors juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    if(HLY_BUILD_TESTS)
        add_executable(hly_processor_tests tests/processor_tests.cpp)
        target_link_libraries(hly_processor_tests PRIVATE HlyChorfeed)
        target_include_directories(hly_processor_tests PRIVATE "$<TARGET_PROPERTY:HlyChorfeed,INCLUDE_DIRECTORIES>")
        target_compile_definitions(hly_processor_tests PRIVATE "$<TARGET_PROPERTY:HlyChorfeed,COMPILE_DEFINITIONS>")
        add_test(NAME processor COMMAND hly_processor_tests)
    endif()
    if(HLY_BUILD_TOOLS)
        juce_add_gui_app(hly_capture PRODUCT_NAME "hly_capture")
        target_sources(hly_capture PRIVATE tests/capture.cpp)
        target_link_libraries(hly_capture PRIVATE HlyChorfeed)
        target_include_directories(hly_capture PRIVATE "$<TARGET_PROPERTY:HlyChorfeed,INCLUDE_DIRECTORIES>")
        get_target_property(hly_capture_definitions HlyChorfeed COMPILE_DEFINITIONS)
        list(FILTER hly_capture_definitions EXCLUDE REGEX "^JUCE_STANDALONE_APPLICATION=")
        target_compile_definitions(hly_capture PRIVATE ${hly_capture_definitions})
    endif()
endif()
