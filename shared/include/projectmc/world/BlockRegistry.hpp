#pragma once
#include "projectmc/world/Block.hpp"
#include <string_view>
#include <unordered_map>
#include <vector>
namespace projectmc::world {
class BlockRegistry {
public:
  BlockRegistry();
  BlockId registerBlock(std::string name, BlockMaterial material, bool solid, bool transparent);
  [[nodiscard]] const BlockDefinition& get(BlockId id) const;
  [[nodiscard]] const BlockDefinition& get(std::string_view name) const;
  [[nodiscard]] BlockId id(std::string_view name) const;
  [[nodiscard]] std::size_t size() const noexcept { return blocks_.size(); }
private:
  std::vector<BlockDefinition> blocks_;
  std::unordered_map<std::string, BlockId> names_;
};
}
