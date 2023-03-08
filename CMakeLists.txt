cmake_minimum_required(VERSION 3.22.1)

set(PROJECT_NAME "Text2Sample")
set(FORMATS Standalone AU VST3)
file(STRINGS VERSION CURRENT_VERSION)
project(${PROJECT_NAME} VERSION ${CURRENT_VERSION})
set(CMAKE_XCODE_GENERATE_SCHEME OFF)

# Build universal binaries only for Release builds
if (APPLE)
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        message("Building universal binary for Release")
        set(CMAKE_OSX_ARCHITECTURES arm64 x86_64)
    else ()
        message("Building x86_64 for Debug")
        set(CMAKE_OSX_ARCHITECTURES x86_64)
    endif ()
endif ()

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Support macOS down to High Sierra")

set_property(GLOBAL PROPERTY USE_FOLDERS YES)
option(JUCE_ENABLE_MODULE_SOURCE_GROUPS "Show all module sources in IDE projects" ON)

add_subdirectory(JUCE)

juce_add_plugin(${PROJECT_NAME}
    COMPANY_NAME "CalGoheenMattRice"
    BUNDLE_ID com.cgmr.text2sample
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Cgmr
    PLUGIN_CODE Ttsm
    FORMATS ${FORMATS}
    PRODUCT_NAME ${PROJECT_NAME}
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

file(GLOB SourceFiles CONFIGURE_DEPENDS 
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
)

target_sources(${PROJECT_NAME} PRIVATE ${SourceFiles})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "" FILES ${SourceFiles})

juce_add_binary_data(Assets SOURCES "Assets/wave.svg")
set_target_properties(Assets 
    PROPERTIES 
        POSITION_INDEPENDENT_CODE TRUE 
        FOLDER "Targets"
)

foreach(target ${FORMATS} "All")
    if(TARGET ${PROJECT_NAME}_${target})
        set_target_properties(${PROJECT_NAME}_${target} 
            PROPERTIES
                FOLDER "Targets"
                XCODE_GENERATE_SCHEME ON
        )
    endif()
endforeach()

set(JUCE_DEPENDENCIES
    juce::juce_audio_utils
    juce::juce_dsp
)

set(ONNX_RUNTIME "${CMAKE_CURRENT_SOURCE_DIR}/ort-builder/libs/macos-arm64_x86_64/onnxruntime.a")

add_subdirectory(resample)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Assets
        ${JUCE_DEPENDENCIES}
        ${ONNX_RUNTIME}
        r8b
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ort-builder/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ort-builder/model
)

juce_generate_juce_header(${PROJECT_NAME})
