#pragma once
#include <array>
#include "projectmc/game/Camera.hpp"

namespace projectmc::game {

struct Mat4 {
 std::array<float,16> m{};
 [[nodiscard]] static Mat4 identity();
 [[nodiscard]] static Mat4 perspective(float verticalFovDegrees,float aspect,float nearPlane,float farPlane);
 [[nodiscard]] static Mat4 view(const Camera& camera);
 [[nodiscard]] Mat4 operator*(const Mat4& rhs) const;
};

struct CameraMatrices {
 Mat4 view{};
 Mat4 projection{};
 Mat4 viewProjection{};
 float nearPlane{0.05f};
 float farPlane{512.0f};

 void update(const Camera& camera,int viewportWidth,int viewportHeight);
};

}
