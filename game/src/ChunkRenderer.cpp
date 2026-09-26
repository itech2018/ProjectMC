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

void ChunkRenderer::appendGpuQuad(IndexedMesh& mesh,const Quad& q,float tileU,float tileV) {
 const unsigned int base=static_cast<unsigned int>(mesh.vertices.size());
 const float tiledUv[4][2]={{0,tileV},{0,0},{tileU,0},{tileU,tileV}};
 for(int i=0;i<4;++i) {
  const auto& p=q.vertices[i];
  mesh.vertices.push_back({p.x,p.y,p.z,tiledUv[i][0],tiledUv[i][1],q.shade,q.transparent?.72f:1.0f,
   q.uv.u0,q.uv.v0,q.uv.u1,q.uv.v1});
 }
 mesh.indices.insert(mesh.indices.end(),{base,base+1,base+2,base,base+2,base+3});
}

void ChunkRenderer::rebuildMesh(const world::ChunkPosition& pos,const world::Chunk& chunk,const world::World& world,const TextureAtlas& atlas) {
 static constexpr std::array<P3,8> corners={{{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}}};
 static constexpr int faces[6][4]={{0,3,2,1},{4,5,6,7},{0,4,7,3},{1,2,6,5},{3,7,6,2},{0,1,5,4}};
 static constexpr int normals[6][3]={{0,0,-1},{0,0,1},{-1,0,0},{1,0,0},{0,1,0},{0,-1,0}};
 static constexpr float shades[6]={.72f,.82f,.68f,.78f,1.0f,.58f};

 ChunkMesh mesh;
 const auto& reg=world.blocks();

 // Keep the compatibility renderer's simple quads, and keep transparent GPU
 // faces unmerged so their existing ordering behaviour remains predictable.
 for(int y=0;y<world::Chunk::Height;++y) for(int z=0;z<world::Chunk::Depth;++z) for(int x=0;x<world::Chunk::Width;++x) {
  const auto id=chunk.get(x,y,z);
  if(!id) continue;
  const auto& def=reg.get(id);
  const int wx=pos.x*world::Chunk::Width+x,wy=pos.y*world::Chunk::Height+y,wz=pos.z*world::Chunk::Depth+z;
  for(int f=0;f<6;++f) {
   const auto neighbour=world.getBlock(wx+normals[f][0],wy+normals[f][1],wz+normals[f][2]);
   if(neighbour&&!reg.get(neighbour).transparent) continue;
   if(neighbour==id&&def.transparent) continue;
   Quad q;
   const std::string_view texture=f==4?def.textures.top:(f==5?def.textures.bottom:def.textures.side);
   q.uv=atlas.region(texture);q.shade=shades[f];q.transparent=def.transparent;
   for(int i=0;i<4;++i){const auto v=corners[faces[f][i]];q.vertices[i]={float(wx)+v.x,float(wy)+v.y,float(wz)+v.z};}
   if(q.transparent){mesh.transparent.push_back(q);appendGpuQuad(mesh.transparentGpu,q);}
   else mesh.opaque.push_back(q);
  }
 }

 // Greedy mesh opaque faces independently for each direction. Adjacent visible
 // faces with the same block ID are collapsed into one rectangle. The shader
 // repeats the atlas tile over the merged rectangle instead of stretching it.
 const int dims[3]={world::Chunk::Width,world::Chunk::Height,world::Chunk::Depth};
 for(int f=0;f<6;++f) {
  int d=f<2?2:(f<4?0:1);
  const bool positive=(f==1||f==3||f==4);
  const int u=(d+1)%3,v=(d+2)%3;
  std::vector<world::BlockId> mask(static_cast<size_t>(dims[u]*dims[v]));
  std::vector<unsigned char> used(mask.size());

  for(int slice=0;slice<dims[d];++slice) {
   std::fill(mask.begin(),mask.end(),0);
   std::fill(used.begin(),used.end(),0);
   for(int vv=0;vv<dims[v];++vv) for(int uu=0;uu<dims[u];++uu) {
    int p[3]{};p[d]=slice;p[u]=uu;p[v]=vv;
    const auto id=chunk.get(p[0],p[1],p[2]);
    if(!id||reg.get(id).transparent) continue;
    const int wx=pos.x*world::Chunk::Width+p[0],wy=pos.y*world::Chunk::Height+p[1],wz=pos.z*world::Chunk::Depth+p[2];
    const auto neighbour=world.getBlock(wx+normals[f][0],wy+normals[f][1],wz+normals[f][2]);
    if(neighbour&&!reg.get(neighbour).transparent) continue;
    mask[static_cast<size_t>(vv*dims[u]+uu)]=id;
   }

   for(int vv=0;vv<dims[v];++vv) for(int uu=0;uu<dims[u];++uu) {
    const size_t at=static_cast<size_t>(vv*dims[u]+uu);
    const auto id=mask[at];
    if(!id||used[at]) continue;
    int width=1;
    while(uu+width<dims[u]&&!used[at+width]&&mask[at+width]==id) ++width;
    int height=1;
    bool grow=true;
    while(vv+height<dims[v]&&grow) {
     for(int k=0;k<width;++k) {
      const size_t test=static_cast<size_t>((vv+height)*dims[u]+uu+k);
      if(used[test]||mask[test]!=id){grow=false;break;}
     }
     if(grow) ++height;
    }
    for(int y2=0;y2<height;++y2) for(int x2=0;x2<width;++x2)
     used[static_cast<size_t>((vv+y2)*dims[u]+uu+x2)]=1;

    int origin[3]{};origin[d]=slice+(positive?1:0);origin[u]=uu;origin[v]=vv;
    float base[3]={float(pos.x*world::Chunk::Width+origin[0]),float(pos.y*world::Chunk::Height+origin[1]),float(pos.z*world::Chunk::Depth+origin[2])};
    float du[3]{};du[u]=float(width);
    float dv[3]{};dv[v]=float(height);
    auto point=[&](float au,float av){return Vertex3{base[0]+du[0]*au+dv[0]*av,base[1]+du[1]*au+dv[1]*av,base[2]+du[2]*au+dv[2]*av};};

    Quad q;
    const auto& def=reg.get(id);
    const std::string_view texture=f==4?def.textures.top:(f==5?def.textures.bottom:def.textures.side);
    q.uv=atlas.region(texture);q.shade=shades[f];q.transparent=false;
    if(positive) q.vertices={point(0,0),point(1,0),point(1,1),point(0,1)};
    else q.vertices={point(0,0),point(0,1),point(1,1),point(1,0)};
    appendGpuQuad(mesh.opaqueGpu,q,float(width),float(height));
   }
  }
 }

 mesh.revision=nextRevision_++;
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

void ChunkRenderer::releaseGpuMeshes(GpuRenderBackend& backend) {
 for(auto& [pos,mesh]:gpuMeshes_) {
  backend.releaseMesh(mesh.opaque);
  backend.releaseMesh(mesh.transparent);
 }
 gpuMeshes_.clear();
}

void ChunkRenderer::syncGpuMeshes(GpuRenderBackend& backend) {
 std::vector<world::ChunkPosition> stale;
 for(const auto& [pos,gpu]:gpuMeshes_) if(!meshes_.contains(pos)) stale.push_back(pos);
 for(const auto& pos:stale) {
  auto it=gpuMeshes_.find(pos);
  if(it!=gpuMeshes_.end()) {
   backend.releaseMesh(it->second.opaque);
   backend.releaseMesh(it->second.transparent);
   gpuMeshes_.erase(it);
  }
 }

 for(const auto& [pos,cpu]:meshes_) {
  auto& gpu=gpuMeshes_[pos];
  if(gpu.revision==cpu.revision) continue;
  backend.releaseMesh(gpu.opaque);
  backend.releaseMesh(gpu.transparent);

  const auto upload=[&](const IndexedMesh& source,GpuRenderBackend::BufferPair& target) {
   if(source.vertices.empty()||source.indices.empty()) return;
   backend.uploadMesh(source.vertices.data(),
    static_cast<Uint32>(source.vertices.size()*sizeof(GpuVertex)),
    source.indices.data(),
    static_cast<Uint32>(source.indices.size()*sizeof(unsigned int)),
    static_cast<Uint32>(source.indices.size()),target);
  };
  upload(cpu.opaqueGpu,gpu.opaque);
  upload(cpu.transparentGpu,gpu.transparent);
  gpu.revision=cpu.revision;
 }
}

void ChunkRenderer::renderGpuWorld(GpuRenderBackend& backend,const Camera& cam,int w,int h) {
 stats_={};
 stats_.loadedChunks=meshes_.size();
 for(const auto& [pos,mesh]:meshes_) {
  stats_.opaqueQuads+=mesh.opaqueGpu.indices.size()/6;
  stats_.transparentQuads+=mesh.transparentGpu.indices.size()/6;
  stats_.gpuVertices+=mesh.opaqueGpu.vertices.size()+mesh.transparentGpu.vertices.size();
  stats_.gpuTriangles+=(mesh.opaqueGpu.indices.size()+mesh.transparentGpu.indices.size())/3;
 }
 if(!backend.worldPipelineReady()||w<=0||h<=0) return;
 const float yaw=cam.yaw*pi/180.0f;
 const float pitch=cam.pitch*pi/180.0f;
 const float forwardX=std::cos(yaw),forwardZ=std::sin(yaw);
 const float rightX=-forwardZ,rightZ=forwardX;
 const float cp=std::cos(pitch),sp=std::sin(pitch);
 const float halfHFov=std::atan(std::tan(cam.fieldOfView*pi/360.0f)*(static_cast<float>(w)/static_cast<float>(h)));

 std::vector<std::pair<float,const DeviceChunkMesh*>> transparentDraws;
 for(const auto& [pos,gpu]:gpuMeshes_) {
  const float minX=static_cast<float>(pos.x*world::Chunk::Width);
  const float minY=static_cast<float>(pos.y*world::Chunk::Height);
  const float minZ=static_cast<float>(pos.z*world::Chunk::Depth);
  const float centerX=minX+world::Chunk::Width*.5f;
  const float centerY=minY+world::Chunk::Height*.5f;
  const float centerZ=minZ+world::Chunk::Depth*.5f;
  const float dx=centerX-cam.position.x,dy=centerY-cam.position.y,dz=centerZ-cam.position.z;
  const float horizontal=dx*forwardX+dz*forwardZ;
  const float cameraDepth=dy*sp+horizontal*cp;
  constexpr float chunkRadius=13.9f;
  if(cameraDepth < -chunkRadius) continue;
  const float cameraRight=dx*rightX+dz*rightZ;
  const float sideLimit=std::max(cameraDepth,0.0f)*std::tan(halfHFov)+chunkRadius;
  if(std::abs(cameraRight)>sideLimit) continue;
  backend.drawIndexed(gpu.opaque);
  ++stats_.renderedChunks;
  if(gpu.transparent.indexCount>0) {
   const float distanceSquared=dx*dx+dy*dy+dz*dz;
   transparentDraws.emplace_back(distanceSquared,&gpu);
  }
 }

 std::sort(transparentDraws.begin(),transparentDraws.end(),
  [](const auto& a,const auto& b){return a.first>b.first;});
 for(const auto& entry:transparentDraws)
  backend.drawIndexed(entry.second->transparent,true);
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
