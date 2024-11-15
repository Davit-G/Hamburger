add_subdirectory(hiir)

target_sources("${BaseTargetName}"
    PRIVATE
        HilbertBiquad.h
        )