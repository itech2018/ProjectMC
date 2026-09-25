#pragma once
#include <cstddef>
namespace projectmc::world {
struct ChunkPosition { int x{}; int y{}; int z{}; auto operator<=>(const ChunkPosition&) const = default; };
struct ChunkPositionHash {
  std::size_t operator()(const ChunkPosition& p) const noexcept {
    auto h = static_cast<std::size_t>(p.x) * 73856093u;
    h ^= static_cast<std::size_t>(p.y) * 19349663u;
    h ^= static_cast<std::size_t>(p.z) * 83492791u;
    return h;
  }
};
}
