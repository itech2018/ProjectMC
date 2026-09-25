#include "projectmc/game/Player.hpp"
#include <algorithm>
#include <cmath>
namespace projectmc::game {
bool Player::collides(const world::World&w,const Vec3&p) const{
 const float h=Width*.5f;
 int minX=(int)std::floor(p.x-h),maxX=(int)std::floor(p.x+h);
 int minY=(int)std::floor(p.y),maxY=(int)std::floor(p.y+Height-.001f);
 int minZ=(int)std::floor(p.z-h),maxZ=(int)std::floor(p.z+h);
 for(int y=minY;y<=maxY;++y)for(int z=minZ;z<=maxZ;++z)for(int x=minX;x<=maxX;++x){
  auto id=w.getBlock(x,y,z);if(id!=0&&w.blocks().get(id).solid)return true;
 }return false;
}
void Player::moveAxis(const world::World&w,float a,int axis){
 if(a==0)return;Vec3 next=position;if(axis==0)next.x+=a;else if(axis==1)next.y+=a;else next.z+=a;
 if(!collides(w,next)){position=next;return;}
 const float step=a>0?.01f:-.01f;
 for(float moved=0;std::abs(moved+step)<=std::abs(a);moved+=step){Vec3 n=position;if(axis==0)n.x+=step;else if(axis==1)n.y+=step;else n.z+=step;if(collides(w,n))break;position=n;}
 if(axis==1){if(a<0)grounded=true;velocity.y=0;}else if(axis==0)velocity.x=0;else velocity.z=0;
}
void Player::update(double dt,const InputState&i,const world::World&w,float yaw){
 float d=std::min((float)dt,.05f),r=yaw*3.1415926535f/180.0f;
 float fx=std::cos(r),fz=std::sin(r),rx=-fz,rz=fx,mx=0,mz=0;
 if(i.forward){mx+=fx;mz+=fz;}if(i.backward){mx-=fx;mz-=fz;}if(i.right){mx+=rx;mz+=rz;}if(i.left){mx-=rx;mz-=rz;}
 float len=std::sqrt(mx*mx+mz*mz);if(len>0){mx/=len;mz/=len;}
 float speed=i.sprint?7.0f:4.5f;velocity.x=mx*speed;velocity.z=mz*speed;
 if(i.jump&&grounded){velocity.y=7.5f;grounded=false;}
 velocity.y=std::max(velocity.y-22.0f*d,-35.0f);grounded=false;
 moveAxis(w,velocity.x*d,0);moveAxis(w,velocity.y*d,1);moveAxis(w,velocity.z*d,2);
}
}
