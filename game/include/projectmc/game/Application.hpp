#pragma once
#include <SDL3/SDL.h>
#include "projectmc/Config.hpp"
#include "projectmc/game/Input.hpp"
#include "projectmc/game/Camera.hpp"
#include "projectmc/game/ChunkRenderer.hpp"
#include "projectmc/game/Player.hpp"
#include "projectmc/game/Raycast.hpp"
#include "projectmc/game/TextureAtlas.hpp"
#include "projectmc/game/RenderBackend.hpp"
#include <memory>
#include <filesystem>
#include <string>
#include "projectmc/world/World.hpp"
#include "projectmc/world/WorldManager.hpp"
#include <vector>
namespace projectmc::game {
class Application {
public:
 explicit Application(projectmc::GameConfig config);~Application();
 Application(const Application&)=delete;Application& operator=(const Application&)=delete;
 bool initialize();int run();
private:
 enum class Screen { Title, Singleplayer, CreateWorld, RenameWorld, DeleteWorld, Playing, Paused };
 void processEvents();void update(double dt);void render();void interact(bool place);
 void activateMenuSelection();void loadWorldEntry(const world::WorldEntry& entry);void saveCurrentWorld();void leaveWorldToMenu();
 projectmc::GameConfig config_;SDL_Window* window_{nullptr};SDL_Renderer* renderer_{nullptr};std::unique_ptr<RenderBackend> renderBackend_;
 InputState input_{};Camera camera_{};Player player_{};TextureAtlas atlas_{};ChunkRenderer chunkRenderer_{};world::World world_{};
 world::BlockId selectedBlock_{1};
  int selectedSlot_{0};
  bool running_{false};
  bool gpuMode_{true};
  double fpsAccumulator_{0.0};
  int fpsFrames_{0};
  double displayedFps_{0.0};
  std::filesystem::path worldPath_{};
  std::string worldName_{"Development World"};
  Screen screen_{Screen::Title};
  int menuSelection_{0};
  std::vector<world::WorldEntry> availableWorlds_{};
  std::string createWorldName_{"New World"};
  std::string createWorldSeed_{};
  int createField_{0};
  int worldListOffset_{0};
  int selectedWorld_{0};
  std::string renameWorldName_{};
  int confirmSelection_{0};
};
}
