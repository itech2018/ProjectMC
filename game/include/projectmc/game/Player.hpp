#pragma once
#include "projectmc/game/Camera.hpp"
#include "projectmc/game/Input.hpp"
#include "projectmc/world/World.hpp"
namespace projectmc::game {
class Player {
public:
 Vec3 position{8.0f,13.0f,12.0f};
 Vec3 velocity{};
 bool grounded{false};
 void update(double dt,const InputState& input,const world::World& world,float yaw);
 [[nodiscard]] Vec3 eyePosition() const { return {position.x,position.y+1.62f,position.z}; }
private:
 static constexpr float Width=.6f, Height=1.8f;
 [[nodiscard]] bool collides(const world::World& world,const Vec3& p) const;
 void moveAxis(const world::World& world,float amount,int axis);
};
}
