#pragma once
#include "projectmc/game/Camera.hpp"
#include "projectmc/world/World.hpp"
namespace projectmc::game {
struct BlockHit { bool hit{false}; int x{},y{},z{},previousX{},previousY{},previousZ{}; };
[[nodiscard]] BlockHit raycastBlocks(const world::World& world,const Camera& camera,float distance=6.0f);
}
