cmake_minimum_required(VERSION 3.22)
project(PultecProto VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PULTEC_BUILD_TESTS  "Build the DSP-core tests (no JUCE needed)" ON)
option(PULTEC_BUILD_PLUGIN "Build the JUCE plugin (downloads JUCE)"  ON)

# Header-only DSP core (shared by tests and plugin)
add_library(pultec_dsp INTERFACE)
target_include_directories(pultec_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cpp/dsp)

if(PULTEC_BUILD_TESTS)
  enable_testing()
  add_executable(pultec_dsp_tests cpp/tests/dsp_test.cpp)
  target_link_libraries(pultec_dsp_tests PRIVATE pultec_dsp)
  add_test(NAME dsp_vs_python
           COMMAND pultec_dsp_tests ${CMAKE_CURRENT_SOURCE_DIR}/cpp/reference/reference.txt)
endif()

if(PULTEC_BUILD_PLUGIN)
  include(FetchContent)
  FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.12
    GIT_SHALLOW    TRUE)
  FetchContent_MakeAvailable(JUCE)

  set(PULTEC_FORMATS VST3 Standalone)
  if(APPLE)
    list(APPEND PULTEC_FORMATS AU)
  endif()

  juce_add_plugin(PultecProto
    COMPANY_NAME               "YourName"
    PLUGIN_MANUFACTURER_CODE   Pdev
    PLUGIN_CODE                Peq1
    FORMATS                    ${PULTEC_FORMATS}
    PRODUCT_NAME               "Pultec Proto"
    IS_SYNTH                   FALSE
    NEEDS_MIDI_INPUT           FALSE
    NEEDS_MIDI_OUTPUT          FALSE
    COPY_PLUGIN_AFTER_BUILD    FALSE)

  target_sources(PultecProto PRIVATE plugin/Source/PluginProcessor.cpp
                                     plugin/Source/PluginProcessor.h
                                     plugin/Source/PluginEditor.cpp
                                     plugin/Source/PluginEditor.h)
  target_include_directories(PultecProto PRIVATE plugin/Source)
  target_compile_definitions(PultecProto PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  target_link_libraries(PultecProto
    PRIVATE pultec_dsp juce::juce_audio_utils juce::juce_dsp
    PUBLIC  juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endif()
