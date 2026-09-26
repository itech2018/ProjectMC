include(FetchContent)

# Use SDL's release archive rather than a shallow Git checkout.
# This avoids FetchContent's Git update step, which can break on some
# Windows/CMake/Git combinations with errors such as an invalid HEAD0 ref.
FetchContent_Declare(SDL3
  URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

add_executable(projectmc-game
  src/main.cpp
  src/Application.cpp
  src/ChunkRenderer.cpp
  src/Player.cpp
  src/Raycast.cpp
  src/RenderBackend.cpp
  src/SdlRenderBackend.cpp
  src/TextureAtlas.cpp
)
target_include_directories(projectmc-game PRIVATE include)
target_link_libraries(projectmc-game PRIVATE projectmc-shared SDL3::SDL3-static)
target_compile_features(projectmc-game PRIVATE cxx_std_20)
