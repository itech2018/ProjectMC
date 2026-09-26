#pragma once
#include <string>
namespace projectmc {
struct GameConfig {
  std::string title{"ProjectMC"};
  int width{1280};
  int height{720};
  bool fullscreen{false};
  bool vsync{true};
  int viewDistance{4};
};
GameConfig loadGameConfig(const std::string& path);
}
