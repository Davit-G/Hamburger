# Hamburger Plugin CMakeLists.txt

include (FetchContent)
Set(FETCHCONTENT_QUIET FALSE)

cmake_minimum_required(VERSION 3.15)
set(ProjectVersion 0.4)
set(PluginName "Hamburger")

set(BaseTargetName Hamburger)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(APPLE)
set (CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version" FORCE)
set (CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE INTERNAL "")
endif()

project(Hamburger VERSION "${ProjectVersion}")

include(CheckCXXCompilerFlag)
if (APPLE) 
    CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_OPT_ARCH_NATIVE_SUPPORTED)
    if (COMPILER_OPT_ARCH_NATIVE_SUPPORTED)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    endif()
else()
    CHECK_CXX_COMPILER_FLAG("/arch:AVX2" COMPILER_OPT_ARCH_AVX2_SUPPORTED)
    if (COMPILER_OPT_ARCH_AVX2_SUPPORTED)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
    endif()
endif()


# if (MSVC)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast")
# elseif (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
# endif()

# if (MSVC)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Ox")
# elseif (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")
# endif()

FetchContent_Declare(
	JUCE
	GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
	GIT_TAG 7.0.11
	GIT_PROGRESS TRUE
    GIT_SHALLOW 1
)

FetchContent_MakeAvailable(JUCE)



if(APPLE)
    set(outdir "../build_mac")
else()
    if(UNIX)
        set(outdir "../build_linux")
    else()
        set(outdir "../build_windows")
    endif()
endif()

 
juce_add_plugin("${BaseTargetName}"
    VERSION "${ProjectVersion}"                             # Set this if the plugin version is different to the project version
    # ICON_BIG ...                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
    # ICON_SMALL ...
    COMPANY_NAME AviaryAudio                          # Specify the name of the plugin's author
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
    PLUGIN_MANUFACTURER_CODE AVRY               # A four-character manufacturer id with at least one upper-case character
    PLUGIN_CODE Hmb2                            # A unique four-character plugin id with exactly one upper-case character
                                                # GarageBand 10.3 requires the first letter to be upper-case, and the remaining letters to be lower-case
    FORMATS VST3 Standalone AU                  # The formats to build. Other valid formats are: AAX Unity VST AU AUv3
    PRODUCT_NAME "${PluginName}"  # The name of the final executable, which can differ from the target name
    VST3_CATEGORIES Fx Distortion Fx Dynamics 
)


# add_custom_command(
#         TARGET ${BaseTargetName} POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy
#                 ${CMAKE_SOURCE_DIR}/${build_path}/${BaseTargetName}_artefacts/Release/CLAP/${PluginName}.clap
#                 ${CMAKE_SOURCE_DIR}/${build_path}/${PluginName}.clap)


target_sources("${BaseTargetName}"
    PRIVATE
        PluginProcessor.cpp
        )

add_subdirectory(dsp)
add_subdirectory(service)

target_compile_features("${BaseTargetName}" PUBLIC cxx_std_17)


target_compile_definitions("${BaseTargetName}" PUBLIC
    PERFETTO=0
	JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
	JUCE_USE_FLAC=0
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
	JUCE_USE_CAMERA=0)


juce_add_binary_data("${BaseTargetName}BinaryData" HEADER_NAME BinaryData.h NAMESPACE BinaryData SOURCES
    Assets/poweron.png
    Assets/poweroff.png
    Assets/HamburgerLogo.png
    Assets/Questrial-Regular.ttf
    Assets/Quicksand-Bold.ttf
    Assets/clap-icon.png
    Assets/vst-icon.png
    Assets/wompwuzhere.png
    Assets/feature1.jpg
    Assets/feature2.jpg
    Assets/feature3.jpg
    Assets/feature4.jpg
    Assets/gplv3-icon.png
    Assets/tick.png
    Assets/bg4.jpg
)
set_target_properties("${BaseTargetName}BinaryData" PROPERTIES POSITION_INDEPENDENT_CODE TRUE)



FetchContent_Declare (melatonin_perfetto
  GIT_REPOSITORY https://github.com/sudara/melatonin_perfetto.git
  GIT_TAG origin/main)

FetchContent_Declare (xsimd 
    GIT_REPOSITORY https://github.com/xtensor-stack/xsimd.git
    GIT_TAG origin/master)
    
FetchContent_Declare (melatonin_inspector
    GIT_REPOSITORY https://github.com/sudara/melatonin_inspector.git
    GIT_TAG origin/main
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/melatonin_inspector)

FetchContent_Declare (clap-juce-extensions
  GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
  GIT_TAG origin/main)

FetchContent_MakeAvailable (melatonin_perfetto xsimd melatonin_inspector clap-juce-extensions)

clap_juce_extensions_plugin(TARGET "${BaseTargetName}"
CLAP_ID "com.AviaryAudio.${PluginName}"
CLAP_FEATURES effect "distortion analog compressor" audio-effect)


target_link_libraries("${BaseTargetName}"
    PRIVATE
        # AudioPluginData           # If we'd created a binary data target, we'd link to it here
        juce::juce_dsp
        juce::juce_audio_utils
        "${BaseTargetName}BinaryData"
        Melatonin::Perfetto
        xsimd
        melatonin_inspector
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

