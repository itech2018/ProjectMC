#pragma once
#include <cstdint>
#include <string>
#include <string_view>
namespace projectmc::world {
using BlockId = std::uint16_t;
enum class BlockMaterial : std::uint8_t { Air, Soil, Stone, Wood, Foliage, Sand, Water, Metal };
struct BlockDefinition {
  BlockId id{};
  std::string name;
  BlockMaterial material{BlockMaterial::Air};
  bool solid{false};
  bool transparent{true};
};
}
