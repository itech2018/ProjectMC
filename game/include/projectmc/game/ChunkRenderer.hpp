#pragma once
#include <SDL3/SDL.h>
#include "projectmc/world/Chunk.hpp"
#include "projectmc/world/BlockRegistry.hpp"
#include "projectmc/game/Camera.hpp"
namespace projectmc::game {
class ChunkRenderer {
public:
 struct Point { float x{}, y{}; bool valid{false}; };
 void render(SDL_Renderer* renderer,const world::Chunk& chunk,const world::BlockRegistry& blocks,const Camera& camera,int viewportWidth,int viewportHeight);
 void renderSelection(SDL_Renderer* renderer,int x,int y,int z,const Camera& camera,int viewportWidth,int viewportHeight);
 [[nodiscard]] Point project(float x,float y,float z,const Camera& camera,int w,int h) const;
};
}
