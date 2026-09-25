#pragma once
#include <SDL3/SDL.h>
#include "projectmc/Config.hpp"
#include "projectmc/game/Input.hpp"
namespace projectmc::game {
class Application {
public:
  explicit Application(projectmc::GameConfig config);
  ~Application();
  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;
  bool initialize();
  int run();
private:
  void processEvents();
  void update(double deltaSeconds);
  void render();
  projectmc::GameConfig config_;
  SDL_Window* window_{nullptr};
  SDL_Renderer* renderer_{nullptr};
  InputState input_{};
  bool running_{false};
};
}
