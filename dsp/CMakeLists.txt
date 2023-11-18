add_subdirectory(Distortions)
add_subdirectory(Noise)
add_subdirectory(Dynamics)

target_sources("${BaseTargetName}"
    PRIVATE
        EnvelopeFollower.cpp
        )
