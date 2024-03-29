# Hamburger Plugin CMakeLists.txt

include (FetchContent)
Set(FETCHCONTENT_QUIET FALSE)


# The first line of any CMake project should be a call to `cmake_minimum_required`, which checks
# that the installed CMake will be able to understand the following CMakeLists, and ensures that
# CMake's behaviour is compatible with the named version. This is a standard CMake command, so more
# information can be found in the CMake docs.

cmake_minimum_required(VERSION 3.15)

# The top-level CMakeLists.txt file for a project must contain a literal, direct call to the
# `project()` command. `project()` sets up some helpful variables that describe source/binary
# directories, and the current project version. This is a standard CMake command.

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


if (MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast")
elseif (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
endif()

if (MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Ox")
elseif (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")
endif()




FetchContent_Declare(
	JUCE
	GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
	GIT_TAG 7.0.10
	GIT_PROGRESS TRUE
    GIT_SHALLOW 1
)

FetchContent_MakeAvailable(JUCE)


# If you've installed JUCE somehow (via a package manager, or directly using the CMake install
# target), you'll need to tell this project that it depends on the installed copy of JUCE. If you've
# included JUCE directly in your source tree (perhaps as a submodule), you'll need to tell CMake to
# include that subdirectory as part of the build.

# find_package(JUCE CONFIG REQUIRED)        # If you've installed JUCE to your system
# or
# add_subdirectory(JUCE)                    # If you've put JUCE in a subdirectory called JUCE

# If you are building a VST2 or AAX plugin, CMake needs to be told where to find these SDKs on your
# system. This setup should be done before calling `juce_add_plugin`.



# juce_set_vst2_sdk_path(...)
# juce_set_aax_sdk_path(...)

# `juce_add_plugin` adds a static library target with the name passed as the first argument
# (AudioPluginExample here). This target is a normal CMake target, but has a lot of extra properties set
# up by default. As well as this shared code static library, this function adds targets for each of
# the formats specified by the FORMATS arguments. This function accepts many optional arguments.
# Check the readme at `docs/CMake API.md` in the JUCE repo for the full list..

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


# `juce_generate_juce_header` will create a JuceHeader.h for a given target, which will be generated
# into your build tree. This should be included with `#include <JuceHeader.h>`. The include path for
# this header will be automatically added to the target. The main function of the JuceHeader is to
# include all your JUCE module headers; if you're happy to include module headers directly, you
# probably don't need to call this.

juce_generate_juce_header("${BaseTargetName}") 

# `target_sources` adds source files to a target. We pass the target that needs the sources as the
# first argument, then a visibility parameter for the sources which should normally be PRIVATE.
# Finally, we supply a list of source files that will be built into the target. This is a standard
# CMake command.

target_sources("${BaseTargetName}"
    PRIVATE
        PluginProcessor.cpp
        )

add_subdirectory(dsp)

target_compile_features("${BaseTargetName}" PUBLIC cxx_std_17)


# `target_compile_definitions` adds some preprocessor definitions to our target. In a Projucer
# project, these might be passed in the 'Preprocessor Definitions' field. JUCE modules also make use
# of compile definitions to switch certain features on/off, so if there's a particular feature you
# need that's not on by default, check the module header for the correct flag to set here. These
# definitions will be visible both to your code, and also the JUCE module code, so for new
# definitions, pick unique names that are unlikely to collide! This is a standard CMake command.

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


# If your target needs extra binary assets, you can add them here. The first argument is the name of
# a new static library target that will include all the binary resources. There is an optional
# `NAMESPACE` argument that can specify the namespace of the generated binary data class. Finally,
# the SOURCES argument should be followed by a list of source files that should be built into the
# static library. These source files can be of any kind (wav data, images, fonts, icons etc.).
# Conversion to binary-data will happen when your target is built. 

juce_add_binary_data("${BaseTargetName}BinaryData" HEADER_NAME BinaryData.h NAMESPACE BinaryData SOURCES
    Assets/poweron.png
    Assets/poweroff.png
    Assets/HamburgerLogo.png
    Assets/HamburgerLogoThick.png
    Assets/Quicksand.ttf
    Assets/Questrial-Regular.ttf
    Assets/tick.png
    Assets/bg3.jpg
    Assets/bg4.jpg
)
set_target_properties("${BaseTargetName}BinaryData" PROPERTIES POSITION_INDEPENDENT_CODE TRUE)


# Melatonin profiling



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

# `target_link_libraries` links libraries and JUCE modules to other libraries or executables. Here,
# we're linking our executable target to the `juce::juce_audio_utils` module. Inter-module
# dependencies are resolved automatically, so `juce_core`, `juce_events` and so on will also be
# linked automatically. If we'd generated a binary data target above, we would need to link to it
# here too. This is a standard CMake command.

target_link_libraries("${BaseTargetName}"
    PRIVATE
        # AudioPluginData           # If we'd created a binary data target, we'd link to it here
        juce::juce_dsp
        juce::juce_audio_utils
        "${BaseTargetName}BinaryData"
        Melatonin::Perfetto
        xsimd
        # melatonin_inspector
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

