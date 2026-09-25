#include "projectmc/world/Chunk.hpp"
#include <stdexcept>
namespace projectmc::world {
Chunk::Chunk() { blocks_.fill(0); }
bool Chunk::inBounds(int x, int y, int z) noexcept {
  return x >= 0 && x < Width && y >= 0 && y < Height && z >= 0 && z < Depth;
}
std::size_t Chunk::index(int x, int y, int z) {
  if (!inBounds(x,y,z)) throw std::out_of_range("Chunk coordinate out of bounds");
  return static_cast<std::size_t>(x + Width * (z + Depth * y));
}
BlockId Chunk::get(int x, int y, int z) const { return blocks_[index(x,y,z)]; }
void Chunk::set(int x, int y, int z, BlockId block) { blocks_[index(x,y,z)] = block; }
}
