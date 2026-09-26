#include "projectmc/world/World.hpp"
#include "projectmc/world/TerrainGenerator.hpp"
#include <cmath>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cstdint>
namespace projectmc::world {
World::World()=default;
void World::reset(std::uint64_t seed){chunks_.clear();dirty_.clear();overrides_.clear();seed_=seed;}
std::size_t World::BlockPositionHash::operator()(const BlockPosition& p)const noexcept {
 std::size_t h=std::hash<int>{}(p.x);
 h^=std::hash<int>{}(p.y)+0x9e3779b9+(h<<6)+(h>>2);
 h^=std::hash<int>{}(p.z)+0x9e3779b9+(h<<6)+(h>>2);
 return h;
}
int World::floorDiv(int v,int d){int q=v/d,r=v%d;if(r<0)--q;return q;}int World::floorMod(int v,int d){int r=v%d;return r<0?r+d:r;}
Chunk& World::chunk(const ChunkPosition&p){return chunks_[p];}const Chunk*World::findChunk(const ChunkPosition&p)const{auto i=chunks_.find(p);return i==chunks_.end()?nullptr:&i->second;}
BlockId World::getBlock(int x,int y,int z)const{if(y<0||y>=Chunk::Height)return 0;int cx=floorDiv(x,Chunk::Width),cz=floorDiv(z,Chunk::Depth);auto*c=findChunk({cx,0,cz});return c?c->get(floorMod(x,Chunk::Width),y,floorMod(z,Chunk::Depth)):0;}
void World::markDirty(const ChunkPosition&p){dirty_.insert(p);}
bool World::setBlock(int x,int y,int z,BlockId b){if(y<0||y>=Chunk::Height)return false;int cx=floorDiv(x,Chunk::Width),cz=floorDiv(z,Chunk::Depth),lx=floorMod(x,Chunk::Width),lz=floorMod(z,Chunk::Depth);chunk({cx,0,cz}).set(lx,y,lz,b);overrides_.insert_or_assign({x,y,z},b);markDirty({cx,0,cz});if(lx==0)markDirty({cx-1,0,cz});if(lx==Chunk::Width-1)markDirty({cx+1,0,cz});if(lz==0)markDirty({cx,0,cz-1});if(lz==Chunk::Depth-1)markDirty({cx,0,cz+1});return true;}
void World::generateChunk(int cx,int cz){
 Chunk c;TerrainGenerator(blocks_,seed_).generate(c,cx,cz);
 for(const auto& [p,id]:overrides_) {
  if(floorDiv(p.x,Chunk::Width)==cx&&floorDiv(p.z,Chunk::Depth)==cz&&p.y>=0&&p.y<Chunk::Height)
   c.set(floorMod(p.x,Chunk::Width),p.y,floorMod(p.z,Chunk::Depth),id);
 }
 chunks_.insert_or_assign({cx,0,cz},std::move(c));markDirty({cx,0,cz});
}
void World::generateTerrain(int radius){for(int z=-radius;z<=radius;++z)for(int x=-radius;x<=radius;++x)generateChunk(x,z);}
bool World::saveOverrides(const std::string& path)const {
 std::error_code ec;
 const auto parent=std::filesystem::path(path).parent_path();
 if(!parent.empty()) std::filesystem::create_directories(parent,ec);
 std::ofstream out(path,std::ios::binary|std::ios::trunc);
 if(!out) return false;
 const char magic[8]={'P','M','C','W','O','R','L','D'};
 const std::uint32_t version=1,count=static_cast<std::uint32_t>(overrides_.size());
 out.write(magic,sizeof(magic));out.write(reinterpret_cast<const char*>(&version),sizeof(version));out.write(reinterpret_cast<const char*>(&count),sizeof(count));
 for(const auto& [p,id]:overrides_) {
  const std::int32_t x=p.x,y=p.y,z=p.z;const std::uint16_t block=id;
  out.write(reinterpret_cast<const char*>(&x),sizeof(x));out.write(reinterpret_cast<const char*>(&y),sizeof(y));out.write(reinterpret_cast<const char*>(&z),sizeof(z));out.write(reinterpret_cast<const char*>(&block),sizeof(block));
 }
 return static_cast<bool>(out);
}
bool World::loadOverrides(const std::string& path) {
 std::ifstream in(path,std::ios::binary);if(!in)return false;
 char magic[8]{};std::uint32_t version=0,count=0;
 in.read(magic,sizeof(magic));in.read(reinterpret_cast<char*>(&version),sizeof(version));in.read(reinterpret_cast<char*>(&count),sizeof(count));
 const char expected[8]={'P','M','C','W','O','R','L','D'};
 if(!in||!std::equal(std::begin(magic),std::end(magic),std::begin(expected))||version!=1||count>10000000)return false;
 std::unordered_map<BlockPosition,BlockId,BlockPositionHash> loaded;
 for(std::uint32_t i=0;i<count;++i) {
  std::int32_t x,y,z;std::uint16_t id;
  in.read(reinterpret_cast<char*>(&x),sizeof(x));in.read(reinterpret_cast<char*>(&y),sizeof(y));in.read(reinterpret_cast<char*>(&z),sizeof(z));in.read(reinterpret_cast<char*>(&id),sizeof(id));
  if(!in)return false;
  loaded.insert_or_assign({x,y,z},static_cast<BlockId>(id));
 }
 overrides_=std::move(loaded);return true;
}
void World::updateStreaming(float px,float pz,int radius){int pcx=floorDiv((int)std::floor(px),Chunk::Width),pcz=floorDiv((int)std::floor(pz),Chunk::Depth);for(int z=pcz-radius;z<=pcz+radius;++z)for(int x=pcx-radius;x<=pcx+radius;++x)if(!findChunk({x,0,z}))generateChunk(x,z);std::vector<ChunkPosition> remove;for(auto&[p,c]:chunks_)if(std::abs(p.x-pcx)>radius+1||std::abs(p.z-pcz)>radius+1)remove.push_back(p);for(auto&p:remove){chunks_.erase(p);dirty_.erase(p);}}
}
