#include "projectmc/game/Raycast.hpp"
#include <cmath>
namespace projectmc::game {
BlockHit raycastBlocks(const world::World&w,const Camera&c,float distance){
 constexpr float pi=3.1415926535f;float yaw=c.yaw*pi/180,pitch=c.pitch*pi/180;
 float dx=std::cos(pitch)*std::cos(yaw),dy=std::sin(pitch),dz=std::cos(pitch)*std::sin(yaw);
 int px=(int)std::floor(c.position.x),py=(int)std::floor(c.position.y),pz=(int)std::floor(c.position.z);
 for(float t=0;t<=distance;t+=.025f){int x=(int)std::floor(c.position.x+dx*t),y=(int)std::floor(c.position.y+dy*t),z=(int)std::floor(c.position.z+dz*t);
  if(w.getBlock(x,y,z)!=0)return {true,x,y,z,px,py,pz};px=x;py=y;pz=z;
 }return {};
}
}
