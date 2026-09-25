#pragma once
#include "projectmc/world/Chunk.hpp"
#include "projectmc/world/BlockRegistry.hpp"
namespace projectmc::world {
class TerrainGenerator {
public:
 explicit TerrainGenerator(const BlockRegistry& blocks):blocks_(blocks){}
 void generate(Chunk& chunk,int chunkX,int chunkZ) const;
private:
 const BlockRegistry& blocks_;
};
}
