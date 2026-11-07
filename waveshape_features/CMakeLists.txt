
add_executable(waveshape_features EXCLUDE_FROM_ALL 
    features.cpp
)

# target_compile_options(waveshape_features PRIVATE 
#     -fsanitize=address 
#     -fno-omit-frame-pointer
#     -g
# )

# target_link_options(waveshape_features PRIVATE 
#     -fsanitize=address
# )

target_compile_features(waveshape_features PRIVATE cxx_std_17)

find_package(Curses REQUIRED)
target_include_directories(waveshape_features PRIVATE ${CURSES_INCLUDE_DIRS})
target_link_libraries(waveshape_features PRIVATE ${CURSES_LIBRARIES})

# one-off analysis tools, see README.md. each includes features.cpp, so they need what it needs
foreach(tool table_resolution loudness_drift loudness_bias_asym asym_strength plugin_cost harmonic_features)
    add_executable(waveshape_${tool} EXCLUDE_FROM_ALL analysis/${tool}.cpp)
    target_compile_features(waveshape_${tool} PRIVATE cxx_std_17)
    target_include_directories(waveshape_${tool} PRIVATE ${CURSES_INCLUDE_DIRS})
    target_link_libraries(waveshape_${tool} PRIVATE ${CURSES_LIBRARIES})
endforeach()
