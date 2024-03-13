target_sources("${BaseTargetName}"
PRIVATE
Fuzz.cpp
PattyFuzz.cpp
Cooked.cpp
SoftClipper.cpp
PostClip.cpp
HardClip.cpp
PhaseDist.cpp
)

add_subdirectory(tube)