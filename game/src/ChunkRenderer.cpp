#include "projectmc/game/ChunkRenderer.hpp"
#include <algorithm>
#include <array>
#include <cmath>
namespace projectmc::game {
namespace {
constexpr float pi=3.14159265358979323846f;
struct P3 { float x,y,z; };
}
ChunkRenderer::Point ChunkRenderer::project(float x,float y,float z,const Camera& c,int w,int h) const {
  x-=c.position.x; y-=c.position.y; z-=c.position.z;
  const float yaw=c.yaw*pi/180.0f, pitch=c.pitch*pi/180.0f;
  const float cy=std::cos(yaw), sy=std::sin(yaw);
  const float rx=cy*x+sy*z, rz=-sy*x+cy*z;
  const float cp=std::cos(pitch), sp=std::sin(pitch);
  const float ry=cp*y-sp*rz, dz=sp*y+cp*rz;
  if(dz<=0.05f) return {};
  const float f=(static_cast<float>(h)*0.5f)/std::tan(c.fieldOfView*pi/360.0f);
  return {w*0.5f+(rx*f/dz),h*0.5f-(ry*f/dz),true};
}
void ChunkRenderer::render(SDL_Renderer* r,const world::Chunk& c,const world::BlockRegistry& reg,const Camera& cam,int w,int h) {
  static constexpr std::array<P3,8> corners={{{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}}};
  static constexpr int faces[6][4]={{0,3,2,1},{4,5,6,7},{0,4,7,3},{1,2,6,5},{3,7,6,2},{0,1,5,4}};
  struct Face { std::array<SDL_FPoint,4> p; float depth; world::BlockMaterial mat; };
  std::vector<Face> draw;
  for(int y=0;y<world::Chunk::Height;++y) for(int z=0;z<world::Chunk::Depth;++z) for(int x=0;x<world::Chunk::Width;++x){
    const auto id=c.get(x,y,z); if(id==0) continue;
    const auto& def=reg.get(id);
    const int n[6][3]={{0,0,-1},{0,0,1},{-1,0,0},{1,0,0},{0,1,0},{0,-1,0}};
    for(int f=0;f<6;++f){
      const int nx=x+n[f][0],ny=y+n[f][1],nz=z+n[f][2];
      if(world::Chunk::inBounds(nx,ny,nz) && !reg.get(c.get(nx,ny,nz)).transparent) continue;
      Face face{}; face.mat=def.material; float d=0; bool ok=true;
      for(int i=0;i<4;++i){ const auto q=corners[faces[f][i]]; auto p=project(x+q.x,y+q.y,z+q.z,cam,w,h); if(!p.valid){ok=false;break;} face.p[i]={p.x,p.y}; const float dx=x+q.x-cam.position.x,dy=y+q.y-cam.position.y,dz=z+q.z-cam.position.z; d+=dx*dx+dy*dy+dz*dz; }
      if(ok){face.depth=d/4;draw.push_back(face);}
    }
  }
  std::sort(draw.begin(),draw.end(),[](const Face&a,const Face&b){return a.depth>b.depth;});
  for(const auto& f:draw){
    switch(f.mat){
      case world::BlockMaterial::Soil: SDL_SetRenderDrawColor(r,90,155,70,255); break;
      case world::BlockMaterial::Stone: SDL_SetRenderDrawColor(r,120,120,125,255); break;
      case world::BlockMaterial::Sand: SDL_SetRenderDrawColor(r,210,195,135,255); break;
      case world::BlockMaterial::Water: SDL_SetRenderDrawColor(r,60,120,210,190); break;
      default: SDL_SetRenderDrawColor(r,150,150,150,255); break;
    }
    SDL_Vertex v[4]{};
    for(int i=0;i<4;++i){v[i].position=f.p[i];v[i].color={1,1,1,1};}
    const int idx[6]={0,1,2,0,2,3};
    SDL_RenderGeometry(r,nullptr,v,4,idx,6);
    SDL_SetRenderDrawColor(r,45,55,45,90);
    for(int i=0;i<4;++i) SDL_RenderLine(r,f.p[i].x,f.p[i].y,f.p[(i+1)%4].x,f.p[(i+1)%4].y);
  }
}
void ChunkRenderer::renderSelection(SDL_Renderer* r,int x,int y,int z,const Camera& cam,int w,int h){
 static constexpr int edges[12][2]={{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
 static constexpr float c[8][3]={{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}};
 Point p[8];for(int i=0;i<8;++i)p[i]=project(x+c[i][0],y+c[i][1],z+c[i][2],cam,w,h);
 SDL_SetRenderDrawColor(r,255,255,255,255);for(auto&e:edges)if(p[e[0]].valid&&p[e[1]].valid)SDL_RenderLine(r,p[e[0]].x,p[e[0]].y,p[e[1]].x,p[e[1]].y);
}
}
