#pragma once
#include "projectmc/world/WorldMetadata.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace projectmc::world {
struct WorldEntry {
 std::string id;
 std::filesystem::path path;
 WorldMetadata metadata;
};
class WorldManager {
public:
 explicit WorldManager(std::filesystem::path savesRoot="saves"):root_(std::move(savesRoot)){}
 [[nodiscard]] std::vector<WorldEntry> listWorlds() const;
 [[nodiscard]] WorldEntry createWorld(const std::string& name,std::uint64_t seed) const;
 [[nodiscard]] WorldEntry ensureDefaultWorld() const;
 [[nodiscard]] static std::string safeWorldId(const std::string& name);
private:
 std::filesystem::path root_;
};
}
