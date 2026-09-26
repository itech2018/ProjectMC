add_library(projectmc-shared STATIC
  src/Version.cpp
  src/Log.cpp
  src/Config.cpp
  src/world/BlockRegistry.cpp
  src/world/Chunk.cpp
  src/world/World.cpp
  src/world/WorldMetadata.cpp
  src/world/TerrainGenerator.cpp
)
target_include_directories(projectmc-shared PUBLIC include)
target_compile_features(projectmc-shared PUBLIC cxx_std_20)
