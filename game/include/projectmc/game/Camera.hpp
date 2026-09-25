#pragma once
namespace projectmc::game {
struct Vec3 { float x{}, y{}, z{}; };
class Camera {
public:
  Vec3 position{8.0f, 10.0f, 22.0f};
  float yaw{-90.0f};
  float pitch{-20.0f};
  float fieldOfView{70.0f};
};
}
