#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include "projectmc/game/RenderBackend.hpp"
#include "projectmc/game/RenderMath.hpp"

namespace projectmc::game {

class GpuRenderBackend final : public RenderBackend {
public:
 ~GpuRenderBackend() override;
 bool initialize(SDL_Window* window) override;
 void resize(int width,int height) override;
 void beginFrame(const Camera& camera) override;
 void endFrame() override;
 [[nodiscard]] const char* name() const noexcept override { return "SDL GPU"; }

 [[nodiscard]] SDL_GPUDevice* device() const noexcept { return device_; }
 [[nodiscard]] const CameraMatrices& matrices() const noexcept { return matrices_; }

private:
 void destroyDepthTarget();
 bool createDepthTarget();

 SDL_Window* window_{nullptr};
 SDL_GPUDevice* device_{nullptr};
 SDL_GPUTexture* depthTexture_{nullptr};
 CameraMatrices matrices_{};
 int width_{0};
 int height_{0};
};

}
