#include "projectmc/game/Application.hpp"
#include "projectmc/Log.hpp"
#include <chrono>
namespace projectmc::game {
Application::Application(projectmc::GameConfig config) : config_(std::move(config)) {}
Application::~Application() {
  if (renderer_) SDL_DestroyRenderer(renderer_);
  if (window_) SDL_DestroyWindow(window_);
  SDL_Quit();
}
bool Application::initialize() {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    projectmc::log(projectmc::LogLevel::Error, SDL_GetError());
    return false;
  }
  SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
  if (config_.fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
  window_ = SDL_CreateWindow(config_.title.c_str(), config_.width, config_.height, flags);
  if (!window_) {
    projectmc::log(projectmc::LogLevel::Error, SDL_GetError());
    return false;
  }
  renderer_ = SDL_CreateRenderer(window_, nullptr);
  if (!renderer_) {
    projectmc::log(projectmc::LogLevel::Error, SDL_GetError());
    return false;
  }
  SDL_SetRenderVSync(renderer_, config_.vsync ? 1 : 0);
  running_ = true;
  projectmc::log(projectmc::LogLevel::Info, "Game window and renderer initialized.");
  return true;
}
void Application::processEvents() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) running_ = false;
    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) running_ = false;
  }
  const bool* keys = SDL_GetKeyboardState(nullptr);
  input_.forward = keys[SDL_SCANCODE_W];
  input_.backward = keys[SDL_SCANCODE_S];
  input_.left = keys[SDL_SCANCODE_A];
  input_.right = keys[SDL_SCANCODE_D];
  input_.jump = keys[SDL_SCANCODE_SPACE];
}
void Application::update(double) {}
void Application::render() {
  SDL_SetRenderDrawColor(renderer_, 88, 160, 220, 255);
  SDL_RenderClear(renderer_);
  SDL_RenderPresent(renderer_);
}
int Application::run() {
  using clock = std::chrono::steady_clock;
  auto previous = clock::now();
  while (running_) {
    const auto now = clock::now();
    const std::chrono::duration<double> elapsed = now - previous;
    previous = now;
    processEvents();
    update(elapsed.count());
    render();
  }
  return 0;
}
}
