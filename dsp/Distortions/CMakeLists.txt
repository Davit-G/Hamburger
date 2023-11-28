target_sources("${BaseTargetName}"
PRIVATE
Fuzz.cpp
PattyFuzz.cpp
Cooked.cpp
SoftClipper.cpp
HardClip.cpp
Grunge.cpp
PhaseDist.cpp
)

add_subdirectory(tube)