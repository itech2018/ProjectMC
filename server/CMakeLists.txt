add_executable(projectmc-server src/main.cpp)
target_link_libraries(projectmc-server PRIVATE projectmc-shared)
target_compile_features(projectmc-server PRIVATE cxx_std_20)
