add_subdirectory(Distortions)
add_subdirectory(Noise)

target_sources("${BaseTargetName}"
    PRIVATE
        EnvelopeFollower.cpp #here???
        Compander.cpp
        )

# add_subdirectory(FrequencyShifting)
