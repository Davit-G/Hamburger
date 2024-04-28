add_subdirectory(Distortions)
add_subdirectory(Noise)
add_subdirectory(Dynamics)
add_subdirectory(PreDistortions)
add_subdirectory(Filtering)

target_sources("${BaseTargetName}"
    PRIVATE
        EnvelopeFollower.cpp
        SVTPTFilter.cpp
        SVTPTFilter.h
        )
