#pragma once
#include <cstdint>
#include <string>

namespace projectmc::world {
struct WorldMetadata {
 std::uint32_t formatVersion{1};
 std::string name{"Development World"};
 std::uint64_t seed{0x504d4301ULL};
 float playerX{8.0f},playerY{13.0f},playerZ{12.0f};
 float yaw{0.0f},pitch{0.0f};
};
bool loadWorldMetadata(const std::string& path,WorldMetadata& metadata);
bool saveWorldMetadata(const std::string& path,const WorldMetadata& metadata);
}
