add_subdirectory(LookAndFeel)
add_subdirectory(Modules)
add_subdirectory(SaturationIcons)

target_sources("${BaseTargetName}"
    PRIVATE
        UpdateChecker.cpp
        UpdateChecker.h
        RectSlider.h
        )