#pragma once
#include "projectmc/world/Block.hpp"
#include <array>
#include <cstddef>
namespace projectmc::world {
class Chunk {
public:
  static constexpr int Width = 16;
  static constexpr int Height = 16;
  static constexpr int Depth = 16;
  static constexpr std::size_t Volume = Width * Height * Depth;
  Chunk();
  [[nodiscard]] BlockId get(int x, int y, int z) const;
  void set(int x, int y, int z, BlockId block);
  [[nodiscard]] static bool inBounds(int x, int y, int z) noexcept;
private:
  [[nodiscard]] static std::size_t index(int x, int y, int z);
  std::array<BlockId, Volume> blocks_{};
};
}
