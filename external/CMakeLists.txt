include(cmake/CPM.cmake)
include(cmake/target_settings.cmake)
include(cmake/SubprojectVersion.cmake)
include(cmake/rust_ffi.cmake)

# Mimalloc =====================================================================================
CPMAddPackage(
    NAME mimalloc
    GITHUB_REPOSITORY microsoft/mimalloc
    GIT_TAG master
    REQUIRED
)
if(${mimalloc_ADDED})
    message(STATUS "Added mimalloc")
else()
    message(FATAL_ERROR "Couldn't add mimalloc")
endif()

# ==============================================================================================
# Absent =======================================================================================
CPMAddPackage(
    NAME absent
    GITHUB_REPOSITORY rvarago/absent
    GIT_TAG master
    REQUIRED
)
if(${absent_ADDED})
    message(STATUS "Added absent")
else()
    message(FATAL_ERROR "Couldn't add absent")
endif()

# ==============================================================================================
# boost ========================================================================================
# CPMAddPackage(
#     NAME boost
#     VERSION 1.77.0
#     GITHUB_REPOSITORY "boostorg/boost"
#     GIT_TAG "boost-1.78.0"
# )
# if(${boost_ADDED})
#     message(STATUS "Added boost")
# else()
#     message(FATAL_ERROR "Couldn't add boost")
# endif()

# ==============================================================================================
# ranges-v3 ====================================================================================
CPMAddPackage(
    NAME ranges-v3
    GITHUB_REPOSITORY "ericniebler/range-v3"
    GIT_TAG "master"
    REQUIRED
)
if(${ranges-v3_ADDED})
    message(STATUS "Added ranges-v3")
else()
    message(FATAL_ERROR "Couldn't add ranges-v3")
endif()

# ==============================================================================================
# Abseil =======================================================================================
CPMAddPackage(
    NAME abseil
    GITHUB_REPOSITORY abseil/abseil-cpp
    GIT_TAG master
    COMMIT e2a571b818faaec4185426a8cf71fd2970674423 
    OPTIONS ABSL_PROPAGATE_CXX_STD ON
    REQUIRED
)
if(${abseil_ADDED})
    message(STATUS "Added abseil-cpp")
else()
    message(FATAL_ERROR "Couldn't add abseil-cpp")
endif()

# ==============================================================================================
# JUCE =========================================================================================

CPMAddPackage(
    NAME JUCE
    GITHUB_REPOSITORY juce-framework/JUCE
    GIT_TAG develop
    COMMIT e9417c645b16bc82f6f68e6084e384d0ce9568b3
    REQUIRED
)
if (${JUCE_ADDED})
    subproject_version(JUCE juce_version)
    message(STATUS "Added JUCE: ${juce_version}")
else()
    message(FATAL_ERROR "Couldn't add add JUCE")
endif()
    
# ==============================================================================================
    
add_library(juce_plugin_modules STATIC)
target_link_libraries(juce_plugin_modules
    PRIVATE
        juce::juce_audio_utils
        juce::juce_opengl
        mimalloc-static
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_link_directories(juce_plugin_modules
    PUBLIC
)

target_compile_definitions(juce_plugin_modules
    PUBLIC
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_USE_DIRECTWRITE=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_JACK=1
        JUCE_ALSA=1
        DONT_SET_USING_JUCE_NAMESPACE
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_AU=1
        JucePlugin_Build_VST3=1
        JUCE_VST3_CAN_REPLACE_VST2=0
    INTERFACE
        $<TARGET_PROPERTY:juce_plugin_modules,COMPILE_DEFINITIONS>
)

target_include_directories(juce_plugin_modules
    PRIVATE
        ${mimalloc_SOURCE_DIR}/include
    INTERFACE
        $<TARGET_PROPERTY:juce_plugin_modules,INCLUDE_DIRECTORIES>
)

# VST ASIO SDK =================================================================================
CPMAddPackage(
    NAME vst_asio_sdk
    URL "https://www.steinberg.net/asiosdk"
)
if(${vst_asio_sdk_ADDED})
    message(STATUS "Added ASIO SDK")
    target_include_directories(juce_plugin_modules PUBLIC ${vst_asio_sdk_SOURCE_DIR}/common)
    target_compile_definitions(juce_plugin_modules PUBLIC JUCE_ASIO=1)
else()
    message(WARNING "Couldnt add vst_asio_sdk - Some features may still work but with limited functionality")
endif()
# ==============================================================================================

add_subdirectory(core)

