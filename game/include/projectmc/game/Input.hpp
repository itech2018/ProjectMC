#pragma once
namespace projectmc::game {
struct InputState {
  bool quitRequested{false};
  bool forward{false};
  bool backward{false};
  bool left{false};
  bool right{false};
  bool jump{false};
};
}
