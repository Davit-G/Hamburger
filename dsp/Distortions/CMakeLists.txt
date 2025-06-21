target_sources("${BaseTargetName}"
PRIVATE
Fuzz.cpp
PattyFuzz.cpp
Cooked.cpp
SoftClipper.cpp
PostClip.cpp
PhaseDist.cpp
DiodeWaveshape.cpp
)

add_subdirectory(tube)
add_subdirectory(hysteresis)