#pragma once
#include "projectmc/world/BlockRegistry.hpp"
#include "projectmc/world/Chunk.hpp"
#include "projectmc/world/ChunkPosition.hpp"
#include <unordered_map>
#include <unordered_set>
namespace projectmc::world {
class World {
public:
 World();[[nodiscard]] Chunk& chunk(const ChunkPosition&);[[nodiscard]] const Chunk* findChunk(const ChunkPosition&)const;
 [[nodiscard]] BlockId getBlock(int,int,int)const;bool setBlock(int,int,int,BlockId);
 [[nodiscard]] BlockRegistry& blocks()noexcept{return blocks_;}[[nodiscard]] const BlockRegistry& blocks()const noexcept{return blocks_;}
 [[nodiscard]] const auto& chunks()const noexcept{return chunks_;}
 void generateTerrain(int radius);void updateStreaming(float playerX,float playerZ,int radius);
 [[nodiscard]] bool isDirty(const ChunkPosition&p)const{return dirty_.contains(p);}
 void clearDirty(const ChunkPosition&p){dirty_.erase(p);}
private:
 static int floorDiv(int,int);static int floorMod(int,int);void generateChunk(int,int);void markDirty(const ChunkPosition&);
 BlockRegistry blocks_;std::unordered_map<ChunkPosition,Chunk,ChunkPositionHash> chunks_;std::unordered_set<ChunkPosition,ChunkPositionHash> dirty_;
};
}
