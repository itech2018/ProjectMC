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
#include "projectmc/game/GpuRenderBackend.hpp"

namespace projectmc::game {
class ChunkRenderer {
public:
 struct Point { float x{}, y{}; bool valid{false}; };
 struct Stats {
  std::size_t loadedChunks{};
  std::size_t renderedChunks{};
  std::size_t opaqueQuads{};
  std::size_t transparentQuads{};
  std::size_t gpuVertices{};
  std::size_t gpuTriangles{};
 };

 void syncMeshes(world::World&, const TextureAtlas&);
 void renderWorld(SDL_Renderer*, const world::World&, const TextureAtlas&, const Camera&, int, int);
 void renderSelection(SDL_Renderer*, int, int, int, const Camera&, int, int);
 void syncGpuMeshes(GpuRenderBackend&);
 void renderGpuWorld(GpuRenderBackend&, const Camera&, int, int);
 void releaseGpuMeshes(GpuRenderBackend&);
 [[nodiscard]] Point project(float, float, float, const Camera&, int, int) const;
 [[nodiscard]] Stats stats() const noexcept { return stats_; }

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
  float atlasU0{},atlasV0{},atlasU1{},atlasV1{};
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
  std::size_t revision{0};
 };
 struct DeviceChunkMesh {
  GpuRenderBackend::BufferPair opaque;
  GpuRenderBackend::BufferPair transparent;
  std::size_t revision{0};
 };

 void rebuildMesh(const world::ChunkPosition&, const world::Chunk&, const world::World&, const TextureAtlas&);
 static void appendGpuQuad(IndexedMesh&, const Quad&, float tileU=1.0f, float tileV=1.0f);
 std::unordered_map<world::ChunkPosition,ChunkMesh,world::ChunkPositionHash> meshes_;
 std::unordered_map<world::ChunkPosition,DeviceChunkMesh,world::ChunkPositionHash> gpuMeshes_;
 std::size_t nextRevision_{1};
 Stats stats_{};
};
}
