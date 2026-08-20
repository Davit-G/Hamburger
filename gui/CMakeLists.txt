add_subdirectory(LookAndFeel)
add_subdirectory(Modules)

target_sources("${BaseTargetName}"
    PRIVATE
        UpdateChecker.cpp
        UpdateChecker.h
        )