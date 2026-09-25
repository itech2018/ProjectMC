add_executable(projectmc-game src/main.cpp)
target_link_libraries(projectmc-game PRIVATE projectmc-shared)
target_compile_features(projectmc-game PRIVATE cxx_std_20)
