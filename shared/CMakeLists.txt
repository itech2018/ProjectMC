add_library(projectmc-shared STATIC src/Version.cpp)
target_include_directories(projectmc-shared PUBLIC include)
target_compile_features(projectmc-shared PUBLIC cxx_std_20)
