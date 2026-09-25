#include "projectmc/world/World.hpp"
namespace projectmc::world {
World::World() = default;
Chunk& World::chunk(const ChunkPosition& position) { return chunks_[position]; }
const Chunk* World::findChunk(const ChunkPosition& position) const {
  const auto it = chunks_.find(position);
  return it == chunks_.end() ? nullptr : &it->second;
}
void World::generateTestWorld() {
  auto& c = chunk({0,0,0});
  const auto stone = blocks_.id("stone");
  const auto dirt = blocks_.id("dirt");
  const auto grass = blocks_.id("grass");
  for (int z=0; z<Chunk::Depth; ++z)
    for (int x=0; x<Chunk::Width; ++x) {
      for (int y=0; y<3; ++y) c.set(x,y,z,stone);
      c.set(x,3,z,dirt);
      c.set(x,4,z,grass);
    }
}
}
