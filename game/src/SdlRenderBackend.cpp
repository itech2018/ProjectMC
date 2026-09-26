#include "projectmc/game/SdlRenderBackend.hpp"

namespace projectmc::game {

bool SdlRenderBackend::initialize(SDL_Window* window) {
 window_=window;
 if(!window_) return false;
 SDL_GetWindowSizeInPixels(window_,&width_,&height_);
 return true;
}

void SdlRenderBackend::resize(int width,int height) {
 width_=width;
 height_=height;
}

void SdlRenderBackend::beginFrame(const Camera&) {
 // World drawing is still performed by ChunkRenderer during the migration.
}

void SdlRenderBackend::endFrame() {
}

}
