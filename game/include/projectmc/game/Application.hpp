#pragma once
#include <SDL3/SDL.h>
#include "projectmc/Config.hpp"
#include "projectmc/game/Input.hpp"
#include "projectmc/game/Camera.hpp"
#include "projectmc/game/ChunkRenderer.hpp"
#include "projectmc/game/Player.hpp"
#include "projectmc/game/Raycast.hpp"
#include "projectmc/world/World.hpp"
namespace projectmc::game {
class Application {
public:
 explicit Application(projectmc::GameConfig config);~Application();
 Application(const Application&)=delete;Application& operator=(const Application&)=delete;
 bool initialize();int run();
private:
 void processEvents();void update(double dt);void render();void interact(bool place);
 projectmc::GameConfig config_;SDL_Window* window_{nullptr};SDL_Renderer* renderer_{nullptr};
 InputState input_{};Camera camera_{};Player player_{};ChunkRenderer chunkRenderer_{};world::World world_{};
 world::BlockId selectedBlock_{1};
  int selectedSlot_{0};
  bool running_{false};
};
}
