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
 [[nodiscard]] BlockId getBlock(int x,int y,int z) const;
 bool setBlock(int x,int y,int z,BlockId block);
 [[nodiscard]] BlockRegistry& blocks() noexcept{return blocks_;}
 [[nodiscard]] const BlockRegistry& blocks() const noexcept{return blocks_;}
 [[nodiscard]] const auto& chunks() const noexcept{return chunks_;}
 void generateTerrain(int radius);
private:
 static int floorDiv(int value,int divisor);
 static int floorMod(int value,int divisor);
 void generateChunk(int chunkX,int chunkZ);
 BlockRegistry blocks_;
 std::unordered_map<ChunkPosition,Chunk,ChunkPositionHash> chunks_;
};
}
