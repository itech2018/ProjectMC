#pragma once
#include "projectmc/game/RenderBackend.hpp"

namespace projectmc::game {

// Transitional backend that owns the render lifecycle while world geometry is
// still drawn through SDL_Renderer. It provides the fallback implementation
// for the new backend architecture.
class SdlRenderBackend final : public RenderBackend {
public:
 bool initialize(SDL_Window* window) override;
 void resize(int width,int height) override;
 void beginFrame(const Camera& camera) override;
 void endFrame() override;
 [[nodiscard]] const char* name() const noexcept override { return "SDL compatibility"; }
 [[nodiscard]] RenderCapabilities capabilities() const noexcept override {
  return {false,false,false,true};
 }

 [[nodiscard]] int width() const noexcept { return width_; }
 [[nodiscard]] int height() const noexcept { return height_; }

private:
 SDL_Window* window_{nullptr};
 int width_{0};
 int height_{0};
};
}
