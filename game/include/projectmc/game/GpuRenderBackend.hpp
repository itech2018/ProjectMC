#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include "projectmc/game/RenderBackend.hpp"
#include "projectmc/game/RenderMath.hpp"
#include "projectmc/game/TextureAtlas.hpp"
#include <string>

namespace projectmc::game {

class GpuRenderBackend final : public RenderBackend {
public:
 ~GpuRenderBackend() override;
 bool initialize(SDL_Window* window) override;
 void resize(int width,int height) override;
 void beginFrame(const Camera& camera) override;
 void endFrame() override;
 [[nodiscard]] const char* name() const noexcept override { return "SDL GPU"; }
 [[nodiscard]] RenderCapabilities capabilities() const noexcept override {
  return {true,true,true,false};
 }

 [[nodiscard]] SDL_GPUDevice* device() const noexcept { return device_; }
 [[nodiscard]] const CameraMatrices& matrices() const noexcept { return matrices_; }

 struct BufferPair {
  SDL_GPUBuffer* vertex{nullptr};
  SDL_GPUBuffer* index{nullptr};
  Uint32 indexCount{0};
 };
 bool uploadAtlas(const TextureAtlas& atlas);
 bool uploadMesh(const void* vertices,Uint32 vertexBytes,const void* indices,Uint32 indexBytes,Uint32 indexCount,BufferPair& out);
 void releaseMesh(BufferPair& mesh);
 void drawIndexed(const BufferPair& mesh,bool transparent=false);
 void drawHud();
 bool createWorldPipeline(SDL_GPUShader* vertexShader,SDL_GPUShader* fragmentShader);
 [[nodiscard]] bool worldPipelineReady() const noexcept { return worldPipeline_!=nullptr; }

private:
 bool loadWorldShaders();
 bool loadHudShaders();
 bool createHudPipeline(SDL_GPUShader* vertexShader,SDL_GPUShader* fragmentShader);
 std::string shaderPath(const char* stem) const;
 void destroyDepthTarget();
 bool createDepthTarget();

 SDL_Window* window_{nullptr};
 SDL_GPUDevice* device_{nullptr};
 SDL_GPUTexture* depthTexture_{nullptr};
 SDL_GPUTexture* atlasTexture_{nullptr};
 SDL_GPUSampler* atlasSampler_{nullptr};
 SDL_GPUCommandBuffer* commandBuffer_{nullptr};
 SDL_GPUTexture* swapchainTexture_{nullptr};
 SDL_GPURenderPass* renderPass_{nullptr};
 SDL_GPUGraphicsPipeline* worldPipeline_{nullptr};
 SDL_GPUGraphicsPipeline* transparentPipeline_{nullptr};
 SDL_GPUGraphicsPipeline* hudPipeline_{nullptr};
 SDL_GPUShader* worldVertexShader_{nullptr};
 SDL_GPUShader* worldFragmentShader_{nullptr};
 SDL_GPUShader* hudVertexShader_{nullptr};
 SDL_GPUShader* hudFragmentShader_{nullptr};
 CameraMatrices matrices_{};
 int width_{0};
 int height_{0};
};

}
