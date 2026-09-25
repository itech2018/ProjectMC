#pragma once
namespace projectmc::game {
struct InputState {
  bool quitRequested{false};
  bool forward{false}, backward{false}, left{false}, right{false};
  bool jump{false}, descend{false}, sprint{false};
  bool removeBlock{false}, placeBlock{false};
  float mouseDeltaX{0}, mouseDeltaY{0};
};
}
