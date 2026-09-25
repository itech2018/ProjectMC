#include "projectmc/Config.hpp"
#include <fstream>
#include <sstream>
namespace projectmc {
GameConfig loadGameConfig(const std::string& path) {
  GameConfig config;
  std::ifstream file(path);
  if (!file) return config;
  std::string line;
  while (std::getline(file, line)) {
    const auto p = line.find('=');
    if (p == std::string::npos) continue;
    const auto key = line.substr(0, p);
    const auto value = line.substr(p + 1);
    if (key == "title") config.title = value;
    else if (key == "width") config.width = std::stoi(value);
    else if (key == "height") config.height = std::stoi(value);
    else if (key == "fullscreen") config.fullscreen = value == "true";
    else if (key == "vsync") config.vsync = value != "false";
  }
  return config;
}
}
