add_subdirectory(LookAndFeel)

target_sources("${BaseTargetName}"
    PRIVATE
        UpdateChecker.cpp
        UpdateChecker.h
        )