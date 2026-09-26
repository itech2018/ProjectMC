#include "projectmc/game/RenderMath.hpp"
#include <cmath>

namespace projectmc::game {
namespace {
constexpr float pi=3.14159265358979323846f;
}

Mat4 Mat4::identity() {
 Mat4 out{};
 out.m[0]=out.m[5]=out.m[10]=out.m[15]=1.0f;
 return out;
}

Mat4 Mat4::perspective(float fov,float aspect,float nearPlane,float farPlane) {
 Mat4 out{};
 const float f=1.0f/std::tan(fov*pi/360.0f);
 out.m[0]=f/aspect;
 out.m[5]=f;
 out.m[10]=(farPlane+nearPlane)/(nearPlane-farPlane);
 out.m[11]=-1.0f;
 out.m[14]=(2.0f*farPlane*nearPlane)/(nearPlane-farPlane);
 return out;
}

Mat4 Mat4::view(const Camera& camera) {
 const float yaw=camera.yaw*pi/180.0f;
 const float pitch=camera.pitch*pi/180.0f;
 const float cy=std::cos(yaw),sy=std::sin(yaw);
 const float cp=std::cos(pitch),sp=std::sin(pitch);

 const float fx=cp*cy, fy=sp, fz=cp*sy;
 const float rx=-sy, ry=0.0f, rz=cy;
 const float ux=-sp*cy, uy=cp, uz=-sp*sy;

 Mat4 out=identity();
 out.m[0]=rx; out.m[4]=ry; out.m[8]=rz;
 out.m[1]=ux; out.m[5]=uy; out.m[9]=uz;
 out.m[2]=-fx;out.m[6]=-fy;out.m[10]=-fz;
 out.m[12]=-(rx*camera.position.x+ry*camera.position.y+rz*camera.position.z);
 out.m[13]=-(ux*camera.position.x+uy*camera.position.y+uz*camera.position.z);
 out.m[14]= fx*camera.position.x+fy*camera.position.y+fz*camera.position.z;
 return out;
}

Mat4 Mat4::operator*(const Mat4& rhs) const {
 Mat4 out{};
 for(int col=0;col<4;++col)
  for(int row=0;row<4;++row)
   for(int k=0;k<4;++k)
    out.m[col*4+row]+=m[k*4+row]*rhs.m[col*4+k];
 return out;
}

void CameraMatrices::update(const Camera& camera,int width,int height) {
 const float aspect=height>0?static_cast<float>(width)/static_cast<float>(height):1.0f;
 view=Mat4::view(camera);
 projection=Mat4::perspective(camera.fieldOfView,aspect,nearPlane,farPlane);
 viewProjection=projection*view;
}

}
