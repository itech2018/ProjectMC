#pragma once
#include "projectmc/world/BlockRegistry.hpp"
#include "projectmc/world/Chunk.hpp"
#include "projectmc/world/ChunkPosition.hpp"
#include <unordered_map>
namespace projectmc::world {
class World {
public:
  World();
  [[nodiscard]] Chunk& chunk(const ChunkPosition& position);
  [[nodiscard]] const Chunk* findChunk(const ChunkPosition& position) const;
  [[nodiscard]] BlockRegistry& blocks() noexcept { return blocks_; }
  [[nodiscard]] const BlockRegistry& blocks() const noexcept { return blocks_; }
  void generateTestWorld();
private:
  BlockRegistry blocks_;
  std::unordered_map<ChunkPosition, Chunk, ChunkPositionHash> chunks_;
};
}
