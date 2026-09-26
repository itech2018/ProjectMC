#pragma once
#include <SDL3/SDL.h>
#include <array>
#include <unordered_map>
#include <vector>
#include "projectmc/world/Chunk.hpp"
#include "projectmc/world/ChunkPosition.hpp"
#include "projectmc/world/BlockRegistry.hpp"
#include "projectmc/world/World.hpp"
#include "projectmc/game/Camera.hpp"
#include "projectmc/game/TextureAtlas.hpp"

namespace projectmc::game {
class ChunkRenderer {
public:
 struct Point { float x{}, y{}; bool valid{false}; };

 void syncMeshes(world::World&, const TextureAtlas&);
 void renderWorld(SDL_Renderer*, const world::World&, const TextureAtlas&, const Camera&, int, int);
 void renderSelection(SDL_Renderer*, int, int, int, const Camera&, int, int);
 [[nodiscard]] Point project(float, float, float, const Camera&, int, int) const;

private:
 struct Vertex3 { float x{}, y{}, z{}; };
 struct Quad {
  std::array<Vertex3,4> vertices{};
  AtlasRegion uv{};
  float shade{1.0f};
  bool transparent{false};
 };
 struct GpuVertex {
  float x{},y{},z{};
  float u{},v{};
  float shade{1.0f};
  float opacity{1.0f};
 };
 struct IndexedMesh {
  std::vector<GpuVertex> vertices;
  std::vector<unsigned int> indices;
 };
 struct ChunkMesh {
  std::vector<Quad> opaque;
  std::vector<Quad> transparent;
  IndexedMesh opaqueGpu;
  IndexedMesh transparentGpu;
 };

 void rebuildMesh(const world::ChunkPosition&, const world::Chunk&, const world::World&, const TextureAtlas&);
 static void appendGpuQuad(IndexedMesh&, const Quad&);
 std::unordered_map<world::ChunkPosition,ChunkMesh,world::ChunkPositionHash> meshes_;
};
}
