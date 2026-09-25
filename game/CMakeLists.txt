include(FetchContent)
FetchContent_Declare(SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.2.22
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3)

add_executable(projectmc-game
  src/main.cpp
  src/Application.cpp
)
target_include_directories(projectmc-game PRIVATE include)
target_link_libraries(projectmc-game PRIVATE projectmc-shared SDL3::SDL3)
target_compile_features(projectmc-game PRIVATE cxx_std_20)
