# delete when bumping to juce 9
set(f src/wrapper/clap-juce-wrapper.cpp)
file(READ ${f} src)
string(REGEX REPLACE "#if JUCE_VERSION >= 0x090000([\r\n]+ +// JUCE 9 embedded peers)" "#if JUCE_VERSION >= 0x080000\\1" src "${src}")

string(REGEX MATCHALL "0x080000[\r\n]+ +// JUCE 9 embedded peers" hits "${src}")
list(LENGTH hits n)
if(NOT n EQUAL 2)
    message(FATAL_ERROR "clap-juce-extensions DPI patch no longer applies (${n}/2 gates found), check ${f}")
endif()

file(WRITE ${f} "${src}")
