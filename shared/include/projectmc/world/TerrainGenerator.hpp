#pragma once
#include "projectmc/world/Chunk.hpp"
#include "projectmc/world/BlockRegistry.hpp"
#include <cstdint>
namespace projectmc::world {
class TerrainGenerator {
public:
 explicit TerrainGenerator(const BlockRegistry& blocks,std::uint64_t seed=0):blocks_(blocks),seed_(seed){}
 void generate(Chunk& chunk,int chunkX,int chunkZ) const;
private:
 const BlockRegistry& blocks_;
 std::uint64_t seed_{};
};
}
