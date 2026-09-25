#include "projectmc/game/ChunkRenderer.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>
#include <vector>
namespace projectmc::game {
namespace {constexpr float pi=3.14159265358979323846f;struct P3{float x,y,z;};}
ChunkRenderer::Point ChunkRenderer::project(float x,float y,float z,const Camera&c,int w,int h)const{
 x-=c.position.x;y-=c.position.y;z-=c.position.z;float yaw=c.yaw*pi/180,pitch=c.pitch*pi/180,cy=std::cos(yaw),sy=std::sin(yaw);
 float rx=cy*x+sy*z,rz=-sy*x+cy*z,cp=std::cos(pitch),sp=std::sin(pitch),ry=cp*y-sp*rz,dz=sp*y+cp*rz;if(dz<=.05f)return{};
 float f=(h*.5f)/std::tan(c.fieldOfView*pi/360);return{w*.5f+rx*f/dz,h*.5f-ry*f/dz,true};
}
void ChunkRenderer::render(SDL_Renderer*r,const world::Chunk&c,const world::ChunkPosition&pos,const world::BlockRegistry&reg,const TextureAtlas&atlas,const Camera&cam,int w,int h){
 static constexpr std::array<P3,8> corners={{{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}}};
 static constexpr int faces[6][4]={{0,3,2,1},{4,5,6,7},{0,4,7,3},{1,2,6,5},{3,7,6,2},{0,1,5,4}};
 static constexpr int n[6][3]={{0,0,-1},{0,0,1},{-1,0,0},{1,0,0},{0,1,0},{0,-1,0}};
 static constexpr float shade[6]={.72f,.82f,.68f,.78f,1.0f,.58f};
 struct Face{std::array<SDL_FPoint,4>p;float depth,shade;AtlasRegion uv;bool water;};
 std::vector<Face> draw;
 for(int y=0;y<world::Chunk::Height;++y)for(int z=0;z<world::Chunk::Depth;++z)for(int x=0;x<world::Chunk::Width;++x){
  auto id=c.get(x,y,z);if(!id)continue;const auto&def=reg.get(id);
  for(int f=0;f<6;++f){int nx=x+n[f][0],ny=y+n[f][1],nz=z+n[f][2];if(world::Chunk::inBounds(nx,ny,nz)){auto nid=c.get(nx,ny,nz);if(nid&&!reg.get(nid).transparent)continue;if(nid==id&&def.transparent)continue;}
   std::string_view tex=f==4?def.textures.top:(f==5?def.textures.bottom:def.textures.side);Face q{};q.uv=atlas.region(tex);q.shade=shade[f];q.water=def.material==world::BlockMaterial::Water;float d=0;bool ok=true;
   for(int i=0;i<4;++i){auto v=corners[faces[f][i]];auto p=project(pos.x*world::Chunk::Width+x+v.x,pos.y*world::Chunk::Height+y+v.y,pos.z*world::Chunk::Depth+z+v.z,cam,w,h);if(!p.valid){ok=false;break;}q.p[i]={p.x,p.y};float dx=pos.x*world::Chunk::Width+x+v.x-cam.position.x,dy=pos.y*world::Chunk::Height+y+v.y-cam.position.y,dz=pos.z*world::Chunk::Depth+z+v.z-cam.position.z;d+=dx*dx+dy*dy+dz*dz;}if(ok){q.depth=d/4;draw.push_back(q);}
  }
 }
 std::sort(draw.begin(),draw.end(),[](auto&a,auto&b){return a.depth>b.depth;});
 for(auto&f:draw){SDL_Vertex v[4]{};const float uv[4][2]={{f.uv.u0,f.uv.v1},{f.uv.u0,f.uv.v0},{f.uv.u1,f.uv.v0},{f.uv.u1,f.uv.v1}};for(int i=0;i<4;++i){v[i].position=f.p[i];v[i].tex_coord={uv[i][0],uv[i][1]};v[i].color={f.shade,f.shade,f.shade,f.water?.72f:1.0f};}const int idx[6]={0,1,2,0,2,3};SDL_RenderGeometry(r,atlas.texture(),v,4,idx,6);}
}
void ChunkRenderer::renderSelection(SDL_Renderer*r,int x,int y,int z,const Camera&cam,int w,int h){
 static constexpr int edges[12][2]={{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
 static constexpr float c[8][3]={{-.002f,-.002f,-.002f},{1.002f,-.002f,-.002f},{1.002f,1.002f,-.002f},{-.002f,1.002f,-.002f},{-.002f,-.002f,1.002f},{1.002f,-.002f,1.002f},{1.002f,1.002f,1.002f},{-.002f,1.002f,1.002f}};
 Point p[8];for(int i=0;i<8;++i)p[i]=project(x+c[i][0],y+c[i][1],z+c[i][2],cam,w,h);SDL_SetRenderDrawColor(r,255,255,255,255);for(auto&e:edges)if(p[e[0]].valid&&p[e[1]].valid)SDL_RenderLine(r,p[e[0]].x,p[e[0]].y,p[e[1]].x,p[e[1]].y);
}
}
