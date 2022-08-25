cmake_minimum_required(VERSION 3.2)
project(Connectrix VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(MAIN_TARGET_NAME ${PROJECT_NAME})

add_subdirectory(external)

set(JUCE_COPY_PLUGIN_AFTER_BUILD "C:/Program Files/Common Files/VST3/MyPlugins")

juce_add_plugin(${MAIN_TARGET_NAME}
   COMPANY_NAME "George Phillips"
   ProductName "${MAIN_TARGET_NAME}"
   PLUGIN_MANUFACTURER_CODE Gpps
   PLUGIN_CODE Cnt6
   VERSION "0.1.0"

   IS_SYNTH FALSE
   NEEDS_MIDI_INPUT True

   FORMATS
      AU
      VST3
      Standalone

   VST3_CATEGORIES
      Fx
      Generator
      Tools
      Analyzer
      Dynamics
      EQ
      Filter
      Mono
      Reverb
      Sampler
      Stereo
      Tools

   EDITOR_WANTS_KEYBOARD_FOCUS True
   PLUGINHOST_AU True
   COPY_PLUGIN_AFTER_BUILD True
)

juce_add_binary_data(
   binary_data
   SOURCES
      "resources/aAutobusOmnibus.ttf"
      "resources/CONNECTRIX-logo.svg"
)

#convert relative paths to absolute (required to prevent warnings v3.21)
cmake_policy(SET "CMP0076" NEW)

juce_generate_juce_header(${MAIN_TARGET_NAME})
add_subdirectory(src)

# ======================================================================================

target_compile_definitions(${MAIN_TARGET_NAME} PUBLIC
   DONT_SET_USING_JUCE_NAMESPACE
)


target_include_directories(${MAIN_TARGET_NAME} PRIVATE
   "external/core"
   ${mimalloc_SOURCE_DIR}/include
)

target_link_libraries(${MAIN_TARGET_NAME}
   PRIVATE
      core
      juce_plugin_modules
      mimalloc-static
      binary_data
)
