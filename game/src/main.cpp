#include "projectmc/Config.hpp"
#include "projectmc/Log.hpp"
#include "projectmc/Version.hpp"
#include "projectmc/game/Application.hpp"

int main() {
  projectmc::log(projectmc::LogLevel::Info, "Starting ProjectMC " + std::string(projectmc::Version::string()));
  auto config = projectmc::loadGameConfig("config/game.cfg");
  projectmc::game::Application app(std::move(config));
  if (!app.initialize()) return 1;
  return app.run();
}
