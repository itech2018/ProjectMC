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
 struct ClipVertex { float x,y,z,u,v; };
 struct DrawFace {
  std::vector<SDL_Vertex> vertices;
  std::vector<int> indices;
  float depth{};
 };
 std::vector<DrawFace> draw;

 const float yaw=cam.yaw*pi/180.0f;
 const float pitch=cam.pitch*pi/180.0f;
 const float forwardX=std::cos(yaw),forwardZ=std::sin(yaw);
 const float rightX=-forwardZ,rightZ=forwardX;
 const float cp=std::cos(pitch),sp=std::sin(pitch);
 constexpr float nearPlane=.08f;

 auto toCamera=[&](const Vertex3& v,float u,float texV) {
  const float x=v.x-cam.position.x,y=v.y-cam.position.y,z=v.z-cam.position.z;
  const float rx=x*rightX+z*rightZ;
  const float horizontal=x*forwardX+z*forwardZ;
  const float ry=y*cp-horizontal*sp;
  const float rz=y*sp+horizontal*cp;
  return ClipVertex{rx,ry,rz,u,texV};
 };

 auto clipNear=[&](std::vector<ClipVertex> poly) {
  std::vector<ClipVertex> out;
  if(poly.empty()) return out;
  ClipVertex previous=poly.back();
  bool previousInside=previous.z>=nearPlane;
  for(const auto& current:poly) {
   const bool currentInside=current.z>=nearPlane;
   if(currentInside!=previousInside) {
    const float t=(nearPlane-previous.z)/(current.z-previous.z);
    out.push_back({
     previous.x+(current.x-previous.x)*t,
     previous.y+(current.y-previous.y)*t,
     nearPlane,
     previous.u+(current.u-previous.u)*t,
     previous.v+(current.v-previous.v)*t
    });
   }
   if(currentInside) out.push_back(current);
   previous=current;
   previousInside=currentInside;
  }
  return out;
 };

 for(const auto& [pos,mesh]:meshes_) {
  // Reject entire chunks before touching individual quads. Use a conservative
  // bounding sphere so chunks near the edge of the view are not popped early.
  const float minX=static_cast<float>(pos.x*world::Chunk::Width);
  const float minY=static_cast<float>(pos.y*world::Chunk::Height);
  const float minZ=static_cast<float>(pos.z*world::Chunk::Depth);
  const float centerX=minX+world::Chunk::Width*.5f;
  const float centerY=minY+world::Chunk::Height*.5f;
  const float centerZ=minZ+world::Chunk::Depth*.5f;
  const float dx=centerX-cam.position.x;
  const float dy=centerY-cam.position.y;
  const float dz=centerZ-cam.position.z;
  const float horizontal=dx*forwardX+dz*forwardZ;
  const float cameraDepth=dy*sp+horizontal*cp;
  constexpr float chunkRadius=13.9f; // conservative radius for a 16^3 chunk
  if(cameraDepth < -chunkRadius) continue;

  // Horizontal frustum test. Keep a generous margin for the chunk sphere.
  const float cameraRight=dx*rightX+dz*rightZ;
  const float halfHFov=std::atan(std::tan(cam.fieldOfView*pi/360.0f)*(static_cast<float>(w)/static_cast<float>(h)));
  const float sideLimit=std::max(cameraDepth,0.0f)*std::tan(halfHFov)+chunkRadius;
  if(std::abs(cameraRight)>sideLimit) continue;

  auto collect=[&](const std::vector<Quad>& source,bool transparent) {
   for(const auto& q:source) {
    const float uv[4][2]={{q.uv.u0,q.uv.v1},{q.uv.u0,q.uv.v0},{q.uv.u1,q.uv.v0},{q.uv.u1,q.uv.v1}};
    std::vector<ClipVertex> polygon;
    polygon.reserve(6);
    float depth=0.0f;
    for(int i=0;i<4;++i) {
     polygon.push_back(toCamera(q.vertices[i],uv[i][0],uv[i][1]));
     const float dx=q.vertices[i].x-cam.position.x,dy=q.vertices[i].y-cam.position.y,dz=q.vertices[i].z-cam.position.z;
     depth+=dx*dx+dy*dy+dz*dz;
    }
    polygon=clipNear(std::move(polygon));
    if(polygon.size()<3) continue;

    DrawFace face;
    face.depth=depth*.25f;
    face.vertices.reserve(polygon.size());
    const float focal=(h*.5f)/std::tan(cam.fieldOfView*pi/360.0f);
    for(const auto& p:polygon) {
     SDL_Vertex v{};
     v.position={w*.5f+p.x*focal/p.z,h*.5f-p.y*focal/p.z};
     v.tex_coord={p.u,p.v};
     v.color={q.shade,q.shade,q.shade,transparent?.72f:1.0f};
     face.vertices.push_back(v);
    }
    for(int i=1;i+1<(int)polygon.size();++i) {
     face.indices.push_back(0);
     face.indices.push_back(i);
     face.indices.push_back(i+1);
    }
    draw.push_back(std::move(face));
   }
  };
  collect(mesh.opaque,false);
  collect(mesh.transparent,true);
 }

 std::sort(draw.begin(),draw.end(),[](const DrawFace& a,const DrawFace& b){return a.depth>b.depth;});
 for(const auto& face:draw)
  SDL_RenderGeometry(r,atlas.texture(),face.vertices.data(),(int)face.vertices.size(),face.indices.data(),(int)face.indices.size());
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
