#pragma once
#include <SDL3/SDL.h>
#include "projectmc/game/Camera.hpp"

namespace projectmc::game {

// Common lifecycle for ProjectMC world-rendering backends.
// The current SDL renderer remains available while the depth-buffered GPU
// backend is brought online incrementally.
struct RenderCapabilities {
 bool ownsPresentation{false};
 bool depthBuffer{false};
 bool gpuChunkBuffers{false};
 bool transparentPass{false};
};

class RenderBackend {
public:
 virtual ~RenderBackend() = default;
 virtual bool initialize(SDL_Window* window) = 0;
 virtual void resize(int width,int height) = 0;
 virtual void beginFrame(const Camera& camera) = 0;
 virtual void endFrame() = 0;
 [[nodiscard]] virtual const char* name() const noexcept = 0;
 [[nodiscard]] virtual RenderCapabilities capabilities() const noexcept = 0;
};

}
