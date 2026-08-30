# Hamburger Plugin CMakeLists.txt

include (FetchContent)
Set(FETCHCONTENT_QUIET FALSE)

set(BUILD_CLAP TRUE)

cmake_minimum_required(VERSION 3.15)
set(ProjectVersion 0.8)
set(PluginName "Hamburger")

set(BaseTargetName Hamburger)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# set(CMAKE_UNITY_BUILD ON)

if(APPLE)
    set (CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version" FORCE)
    if(NOT DEFINED CMAKE_OSX_ARCHITECTURES OR CMAKE_OSX_ARCHITECTURES STREQUAL "")
        # Default to native architecture for local builds
        execute_process(COMMAND uname -m OUTPUT_VARIABLE HOST_ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
        set(CMAKE_OSX_ARCHITECTURES "${HOST_ARCH}" CACHE STRING "" FORCE)
    endif()

    # dSYM files for Xcode release builds
    # using ninja will mean these dont apply
    # set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
    set(CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS[variant=Release] "YES")
    set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Release] "dwarf-with-dsym")
    set(CMAKE_XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING[variant=Release] "YES")
    set(CMAKE_XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR "$(inherited)")
    set(CMAKE_XCODE_ATTRIBUTE_DWARF_DSYM_FILE_SHOULD_ACCOMPANY_PRODUCT "YES")
endif()

project(Hamburger VERSION "${ProjectVersion}")


FetchContent_Declare(
	JUCE
	GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
	GIT_TAG 2cdfca8feb300fb424002ba2c2751569e5bacb64
	GIT_PROGRESS TRUE
    GIT_SHALLOW 1
)

FetchContent_MakeAvailable(JUCE)

# set commit hash for use in general idk
execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)


if(APPLE)
    # distinction between arm and x86
    if(CMAKE_OSX_ARCHITECTURES MATCHES "x86_64")
        set(outdir "../build_mac_intel")
    else()
        set(outdir "../build_mac")
    endif()
else()
    if(UNIX)
        set(outdir "../build_linux")
    else()
        set(outdir "../build_windows")
    endif()
endif()


juce_add_plugin("${BaseTargetName}"
    VERSION "${ProjectVersion}"                 # Set this if the plugin version is different to the project version
    PRODUCT_NAME "${PluginName}"                # The name of the final executable, which can differ from the target name
    PLUGIN_MANUFACTURER_CODE Avry               # A four-character manufacturer id with at least one upper-case character
    PLUGIN_CODE Hmb2                            # A unique four-character plugin id with exactly one upper-case character
    DESCRIPTION "Hamburger is a distortion plugin, capable of both subtle tangy harmonics and absolute annihilation and noise-wall-ification to any sound."
                                                # GarageBand 10.3 requires the first letter to be upper-case, and the remaining letters to be lower-case
    # ICON_BIG ...                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
    # ICON_SMALL ...
    COMPANY_NAME "AviaryAudio"                          # Specify the name of the plugin's author
    COMPANY_WEBSITE "https://aviaryaudio.com"
    COMPANY_EMAIL "dave@aviaryaudio.com"
    IS_SYNTH FALSE                       # Is this a synth or an effect?
    NEEDS_MIDI_INPUT FALSE               # Does the plugin need midi input?
    NEEDS_MIDI_OUTPUT FALSE              # Does the plugin need midi output?
    IS_MIDI_EFFECT FALSE                 # Is this plugin a MIDI effect?
    # EDITOR_WANTS_KEYBOARD_FOCUS TRUE/FALSE    # Does the editor need keyboard focus?

    COPY_PLUGIN_AFTER_BUILD TRUE        # Should the plugin be installed to a default location after building?
    VST3_COPY_DIR "${outdir}"
	VST_COPY_DIR "${outdir}"
	AU_COPY_DIR "${outdir}"
    FORMATS VST3 Standalone AU                  # The formats to build. Other valid formats are: AAX Unity VST AU AUv3
    VST3_CATEGORIES Fx Distortion Dynamics
    LV2URI "https://aviaryaudio.com/plugins/hamburgerv2"
)

target_sources("${BaseTargetName}"
    PRIVATE
        PluginProcessor.cpp
        )

add_subdirectory(dsp)
add_subdirectory(gui)
add_subdirectory(service)

target_compile_features("${BaseTargetName}" PUBLIC cxx_std_17)

target_compile_options(Hamburger PRIVATE
    $<$<AND:$<CONFIG:Release>,$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>>:
        -O3 -ffast-math -flto>
    $<$<CXX_COMPILER_ID:MSVC>:
        /O2 /fp:fast /GL>
)


target_compile_definitions("${BaseTargetName}" PUBLIC
    PERFETTO=0
    #DONT_SET_USING_JUCE_NAMESPACE=1 # if i ever want to remove juce as being included in the namespace by default
	JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
	JUCE_USE_FLAC=0
    JUCE_DSP_ENABLE_SIMD=1
	JUCE_USE_OGGVORBIS=0
	JUCE_USE_MP3AUDIOFORMAT=0
	JUCE_USE_LAME_AUDIO_FORMAT=0
	JUCE_USE_WINDOWS_MEDIA_FORMAT=0
	JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_ENABLE_REPAINT_DEBUGGING=0
    JUCE_COREGRAPHICS_RENDER_WITH_MULTIPLE_PAINT_CALLS=1
    JUCE_COREGRAPHICS_DRAW_ASYNC=1
	JUCE_USE_CURL=0
	JUCE_LOAD_CURL_SYMBOLS_LAZILY=1
	JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_BUILD_EXTRAS=0
    JUCE_BUILD_EXAMPLES=0
	JUCE_WEB_BROWSER=0
	JUCE_USE_WIN_WEBVIEW2=0
	JUCE_USE_CAMERA=0
    GIT_HASH="${GIT_HASH}"
)

juce_add_binary_data("${BaseTargetName}BinaryData" HEADER_NAME BinaryData.h NAMESPACE BinaryData SOURCES
    Assets/poweron.png
    Assets/poweroff.png
    Assets/lockon.png
    Assets/lockoff.png
    Assets/HamburgerLogo.png
    Assets/Questrial-Regular.ttf
    Assets/Quicksand-Bold.ttf
    Assets/OverpassMono-Regular.ttf
    Assets/clap-icon.png
    Assets/vst-icon.png
    Assets/wompwuzhere.png
    Assets/gplv3-icon.png
    Assets/tick.png
    Assets/bg4.jpg
    Assets/Icons/FilmReel.svg
    Assets/Icons/Flask.svg
    Assets/Icons/Grill.svg
    Assets/Icons/Slew.svg
    Assets/Icons/Tube.svg
    Assets/Icons/Waves.svg
)
set_target_properties("${BaseTargetName}BinaryData" PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

FetchContent_Declare (chowdsp_utils
  GIT_REPOSITORY https://github.com/Chowdhury-DSP/chowdsp_utils.git
  GIT_SHALLOW 1
  GIT_TAG origin/master)

FetchContent_MakeAvailable (chowdsp_utils)

FetchContent_Declare (melatonin_perfetto
  GIT_REPOSITORY https://github.com/sudara/melatonin_perfetto.git
  GIT_SHALLOW 1
  GIT_TAG origin/main)

FetchContent_Declare (xsimd
  GIT_REPOSITORY https://github.com/xtensor-stack/xsimd.git
  GIT_SHALLOW 1
  GIT_TAG origin/master)


# FetchContent_Declare (melatonin_inspector
#     GIT_REPOSITORY https://github.com/sudara/melatonin_inspector.git
#     GIT_SHALLOW 1
#     GIT_TAG origin/main
#     SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/melatonin_inspector)

FetchContent_Declare (clap-juce-extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_SHALLOW 1
    GIT_TAG origin/main)

FetchContent_MakeAvailable (clap-juce-extensions)
FetchContent_MakeAvailable (xsimd)

if (PERFETTO)
FetchContent_MakeAvailable (melatonin_perfetto)
endif()

if (BUILD_CLAP)
    clap_juce_extensions_plugin(TARGET "${BaseTargetName}"
    CLAP_ID "com.AviaryAudio.${PluginName}"
    CLAP_FEATURES effect "distortion analog compressor" audio-effect)
endif()

target_link_libraries("${BaseTargetName}"
    PRIVATE
        juce::juce_dsp
        juce::juce_audio_utils
        "${BaseTargetName}BinaryData"
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
        chowdsp::chowdsp_filters
        chowdsp_simd
        xsimd
)
