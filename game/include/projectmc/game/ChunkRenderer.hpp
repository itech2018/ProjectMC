#pragma once
#include <SDL3/SDL.h>
#include "projectmc/world/Chunk.hpp"
#include "projectmc/world/BlockRegistry.hpp"
#include "projectmc/game/Camera.hpp"
#include "projectmc/game/TextureAtlas.hpp"
namespace projectmc::game {
class ChunkRenderer {
public:
 struct Point{float x{},y{};bool valid{false};};
 void render(SDL_Renderer*,const world::Chunk&,const world::ChunkPosition&,const world::BlockRegistry&,const TextureAtlas&,const Camera&,int,int);
 void renderSelection(SDL_Renderer*,int,int,int,const Camera&,int,int);
 [[nodiscard]] Point project(float,float,float,const Camera&,int,int)const;
};
}
