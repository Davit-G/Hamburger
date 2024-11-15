# Hamburger Plugin CMakeLists.txt

include (FetchContent)
Set(FETCHCONTENT_QUIET FALSE)

cmake_minimum_required(VERSION 3.15)
set(ProjectVersion 0.5.0)
set(PluginName "Hamburger")

set(BaseTargetName Hamburger)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(APPLE)
    set (CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version" FORCE)
    set (CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE INTERNAL "")

    # dSYM files for Xcode release builds
    # using ninja will mean these dont apply
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
    set(CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS[variant=Release] "YES")
    set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Release] "dwarf-with-dsym")
    set(CMAKE_XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING[variant=Release] "YES")
    set(CMAKE_XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR "$(inherited)")
    set(CMAKE_XCODE_ATTRIBUTE_DWARF_DSYM_FILE_SHOULD_ACCOMPANY_PRODUCT "YES")

    # find_program(DSYMUTIL_PROGRAM dsymutil)
    # if (DSYMUTIL_PROGRAM)
    # foreach(lang C CXX)
    #     foreach(var LINK_EXECUTABLE CREATE_SHARED_LIBRARY)
    #     set(CMAKE_${lang}_${var} "${CMAKE_${lang}_${var}}" "${DSYMUTIL_PROGRAM} ./Hamburger_artefacts/Release/")
    #     endforeach()
    # endforeach()
    # endif()
endif()

# pdb files for windows release builds
# not needed if we're not using sentry
# if (WIN32)
#     set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
#     set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")

#     # because separate targets (vst3 / clap / etc) are built, we do module linker flags and not shared ones
#     set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
# else()
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
# endif()

# project(Hamburger VERSION "${ProjectVersion}")

# include(CheckCXXCompilerFlag)
# if (APPLE) 
#     CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_OPT_ARCH_NATIVE_SUPPORTED)
#     if (COMPILER_OPT_ARCH_NATIVE_SUPPORTED)
#         set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
#     endif()
# else()
#     # removed because we dont need AVX2 support tbh
#     # CHECK_CXX_COMPILER_FLAG("/arch:AVX2" COMPILER_OPT_ARCH_AVX2_SUPPORTED)
#     # if (COMPILER_OPT_ARCH_AVX2_SUPPORTED)
#     #     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
#     # endif()
# endif()


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

# set commit hash for use in general idk
execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)


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
    #DONT_SET_USING_JUCE_NAMESPACE=1 # if i ever want to remove juce as being included in the namespace by default
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
	JUCE_USE_CAMERA=0
    GIT_HASH="${GIT_HASH}"
)

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
  GIT_SHALLOW 1
  GIT_TAG origin/main)
    
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

if (PERFETTO)
FetchContent_MakeAvailable (melatonin_perfetto)
endif()


clap_juce_extensions_plugin(TARGET "${BaseTargetName}"
CLAP_ID "com.AviaryAudio.${PluginName}"
CLAP_FEATURES effect "distortion analog compressor" audio-effect)

target_link_libraries("${BaseTargetName}"
    PRIVATE
        juce::juce_dsp
        juce::juce_audio_utils
        "${BaseTargetName}BinaryData"
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

