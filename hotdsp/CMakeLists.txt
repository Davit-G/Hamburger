# the dylibs the debug only DLL distortion hot reloads, see HotDsp.h. every folder in here with an initdsp.cpp is one,
# named after its folder: hotdsp/fuzz/initdsp.cpp builds build/hotdsp/fuzz_dsplib.dylib, hotdsp/fuzz/v2/ builds fuzz_v2_dsplib.dylib.
# the target is named the same. out of the plugin's build, one at a time or all of them:
#   cmake --build build --target fuzz_dsplib
#   cmake --build build --target HotDsp
# then pick the dylib in the DLL box. rebuilding it swaps it in live

# juce_dsp and what it needs (audio_formats, audio_basics, core), compiled once here. linked privately so its module
# sources stay here, HotDsp only gets the headers and settings below
add_library(HotDspJuce STATIC EXCLUDE_FROM_ALL)
target_link_libraries(HotDspJuce PRIVATE juce::juce_dsp)
target_include_directories(HotDspJuce PUBLIC "${juce_SOURCE_DIR}/modules")
target_compile_definitions(HotDspJuce PUBLIC
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_MODULE_AVAILABLE_juce_core=1
    JUCE_MODULE_AVAILABLE_juce_audio_basics=1
    JUCE_MODULE_AVAILABLE_juce_audio_formats=1
    JUCE_MODULE_AVAILABLE_juce_dsp=1
    $<IF:$<CONFIG:Debug>,DEBUG=1 _DEBUG=1,NDEBUG=1 _NDEBUG=1>
    JUCE_USE_FLAC=0
    JUCE_USE_OGGVORBIS=0
    JUCE_USE_MP3AUDIOFORMAT=0
    JUCE_USE_LAME_AUDIO_FORMAT=0
    JUCE_USE_WINDOWS_MEDIA_FORMAT=0
    JUCE_USE_CURL=0
    JUCE_DSP_ENABLE_SIMD=1
)
set_target_properties(HotDspJuce PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

# a new folder is picked up on the next build, no reconfigure needed
file(GLOB_RECURSE HotDspInits CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*/initdsp.cpp")

add_custom_target(HotDsp)

foreach(init ${HotDspInits})
    get_filename_component(folder "${init}" DIRECTORY)
    file(RELATIVE_PATH name "${CMAKE_CURRENT_SOURCE_DIR}" "${folder}")
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" name "${name}")
    set(name "${name}_dsplib")

    # the folder's other .cpp files go in too, but not those of any dsp nested under it
    file(GLOB sources CONFIGURE_DEPENDS "${folder}/*.cpp")

    add_library(${name} SHARED EXCLUDE_FROM_ALL ${sources})
    target_compile_features(${name} PRIVATE cxx_std_17)
    target_include_directories(${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}") # #include "HotDsp.h" from any depth
    # only hotdspCreate goes out, the dylib's own copy of juce stays its own
    set_target_properties(${name} PROPERTIES PREFIX "" CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN TRUE)
    target_link_libraries(${name} PRIVATE HotDspJuce)
    add_dependencies(HotDsp ${name})
endforeach()
