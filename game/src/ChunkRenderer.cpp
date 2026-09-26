#include "projectmc/game/ChunkRenderer.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace projectmc::game {
namespace {
constexpr float pi=3.14159265358979323846f;
struct P3 { float x,y,z; };
}

ChunkRenderer::Point ChunkRenderer::project(float x,float y,float z,const Camera& c,int w,int h) const {
 x-=c.position.x; y-=c.position.y; z-=c.position.z;
 const float yaw=c.yaw*pi/180.0f, pitch=c.pitch*pi/180.0f;
 const float forwardX=std::cos(yaw), forwardZ=std::sin(yaw);
 const float rightX=-forwardZ, rightZ=forwardX;
 const float rx=x*rightX+z*rightZ;
 const float horizontal=x*forwardX+z*forwardZ;
 const float cp=std::cos(pitch), sp=std::sin(pitch);
 const float ry=y*cp-horizontal*sp;
 const float dz=y*sp+horizontal*cp;
 if(dz<=.05f) return {};
 const float f=(h*.5f)/std::tan(c.fieldOfView*pi/360.0f);
 return {w*.5f+rx*f/dz,h*.5f-ry*f/dz,true};
}

void ChunkRenderer::rebuildMesh(const world::ChunkPosition& pos,const world::Chunk& chunk,const world::World& world,const TextureAtlas& atlas) {
 static constexpr std::array<P3,8> corners={{{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}}};
 static constexpr int faces[6][4]={{0,3,2,1},{4,5,6,7},{0,4,7,3},{1,2,6,5},{3,7,6,2},{0,1,5,4}};
 static constexpr int normals[6][3]={{0,0,-1},{0,0,1},{-1,0,0},{1,0,0},{0,1,0},{0,-1,0}};
 static constexpr float shades[6]={.72f,.82f,.68f,.78f,1.0f,.58f};

 ChunkMesh mesh;
 const auto& reg=world.blocks();
 for(int y=0;y<world::Chunk::Height;++y) for(int z=0;z<world::Chunk::Depth;++z) for(int x=0;x<world::Chunk::Width;++x) {
  const auto id=chunk.get(x,y,z);
  if(!id) continue;
  const auto& def=reg.get(id);
  const int wx=pos.x*world::Chunk::Width+x;
  const int wy=pos.y*world::Chunk::Height+y;
  const int wz=pos.z*world::Chunk::Depth+z;

  for(int f=0;f<6;++f) {
   const auto neighbour=world.getBlock(wx+normals[f][0],wy+normals[f][1],wz+normals[f][2]);
   if(neighbour&&!reg.get(neighbour).transparent) continue;
   if(neighbour==id&&def.transparent) continue;

   Quad q;
   const std::string_view texture=f==4?def.textures.top:(f==5?def.textures.bottom:def.textures.side);
   q.uv=atlas.region(texture);
   q.shade=shades[f];
   q.transparent=def.transparent;
   for(int i=0;i<4;++i) {
    const auto v=corners[faces[f][i]];
    q.vertices[i]={static_cast<float>(wx)+v.x,static_cast<float>(wy)+v.y,static_cast<float>(wz)+v.z};
   }
   (q.transparent?mesh.transparent:mesh.opaque).push_back(q);
  }
 }
 meshes_.insert_or_assign(pos,std::move(mesh));
}

void ChunkRenderer::syncMeshes(world::World& world,const TextureAtlas& atlas) {
 std::vector<world::ChunkPosition> stale;
 for(const auto& [pos,mesh]:meshes_) if(!world.findChunk(pos)) stale.push_back(pos);
 for(const auto& pos:stale) meshes_.erase(pos);

 for(const auto& [pos,chunk]:world.chunks()) {
  if(!meshes_.contains(pos)||world.isDirty(pos)) {
   rebuildMesh(pos,chunk,world,atlas);
   world.clearDirty(pos);
  }
 }
}

void ChunkRenderer::renderWorld(SDL_Renderer* r,const world::World&,const TextureAtlas& atlas,const Camera& cam,int w,int h) {
 struct DrawFace {
  std::array<SDL_FPoint,4> points{};
  float depth{};
  float shade{};
  AtlasRegion uv{};
  bool transparent{};
 };
 std::vector<DrawFace> draw;

 const float yaw=cam.yaw*pi/180.0f;
 const float pitch=cam.pitch*pi/180.0f;
 const float forwardX=std::cos(yaw),forwardZ=std::sin(yaw);
 const float cp=std::cos(pitch),sp=std::sin(pitch);

 for(const auto& [pos,mesh]:meshes_) {
  auto collect=[&](const std::vector<Quad>& source,bool isTransparent) {
   for(const auto& q:source) {
    DrawFace face;
    face.uv=q.uv;
    face.shade=q.shade;
    face.transparent=isTransparent;
    float depth=0.0f;
    bool visible=true;

    // The SDL prototype has no polygon clipper. If any corner crosses the
    // near plane, skip the quad instead of projecting it into a huge polygon.
    // The upcoming GPU renderer will clip these triangles properly.
    for(const auto& v:q.vertices) {
     const float x=v.x-cam.position.x;
     const float y=v.y-cam.position.y;
     const float z=v.z-cam.position.z;
     const float horizontal=x*forwardX+z*forwardZ;
     const float cameraDepth=y*sp+horizontal*cp;
     if(cameraDepth<=0.12f) { visible=false; break; }
    }
    if(!visible) continue;

    for(int i=0;i<4;++i) {
     const auto& v=q.vertices[i];
     const auto p=project(v.x,v.y,v.z,cam,w,h);
     if(!p.valid) { visible=false; break; }
     face.points[i]={p.x,p.y};
     const float dx=v.x-cam.position.x,dy=v.y-cam.position.y,dz=v.z-cam.position.z;
     depth+=dx*dx+dy*dy+dz*dz;
    }
    if(!visible) continue;
    face.depth=depth*.25f;
    draw.push_back(face);
   }
  };
  collect(mesh.opaque,false);
  collect(mesh.transparent,true);
 }

 // With no hardware depth buffer, all surfaces must participate in the same
 // painter ordering. Rendering water in a separate pass incorrectly painted
 // distant water over nearer terrain.
 std::sort(draw.begin(),draw.end(),[](const DrawFace& a,const DrawFace& b){
  return a.depth>b.depth;
 });

 for(const auto& f:draw) {
  SDL_Vertex v[4]{};
  const float uv[4][2]={{f.uv.u0,f.uv.v1},{f.uv.u0,f.uv.v0},{f.uv.u1,f.uv.v0},{f.uv.u1,f.uv.v1}};
  for(int i=0;i<4;++i) {
   v[i].position=f.points[i];
   v[i].tex_coord={uv[i][0],uv[i][1]};
   v[i].color={f.shade,f.shade,f.shade,f.transparent?.72f:1.0f};
  }
  const int indices[6]={0,1,2,0,2,3};
  SDL_RenderGeometry(r,atlas.texture(),v,4,indices,6);
 }
}
void ChunkRenderer::renderSelection(SDL_Renderer* r,int x,int y,int z,const Camera& cam,int w,int h) {
 static constexpr int edges[12][2]={{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
 static constexpr float corners[8][3]={{-.002f,-.002f,-.002f},{1.002f,-.002f,-.002f},{1.002f,1.002f,-.002f},{-.002f,1.002f,-.002f},{-.002f,-.002f,1.002f},{1.002f,-.002f,1.002f},{1.002f,1.002f,1.002f},{-.002f,1.002f,1.002f}};
 Point points[8];
 for(int i=0;i<8;++i) points[i]=project(x+corners[i][0],y+corners[i][1],z+corners[i][2],cam,w,h);
 SDL_SetRenderDrawColor(r,255,255,255,255);
 for(const auto& edge:edges) if(points[edge[0]].valid&&points[edge[1]].valid)
  SDL_RenderLine(r,points[edge[0]].x,points[edge[0]].y,points[edge[1]].x,points[edge[1]].y);
}
}
