#include "projectmc/world/World.hpp"
#include "projectmc/world/TerrainGenerator.hpp"
#include <cmath>
#include <vector>
namespace projectmc::world {
World::World()=default;int World::floorDiv(int v,int d){int q=v/d,r=v%d;if(r<0)--q;return q;}int World::floorMod(int v,int d){int r=v%d;return r<0?r+d:r;}
Chunk& World::chunk(const ChunkPosition&p){return chunks_[p];}const Chunk*World::findChunk(const ChunkPosition&p)const{auto i=chunks_.find(p);return i==chunks_.end()?nullptr:&i->second;}
BlockId World::getBlock(int x,int y,int z)const{if(y<0||y>=Chunk::Height)return 0;int cx=floorDiv(x,Chunk::Width),cz=floorDiv(z,Chunk::Depth);auto*c=findChunk({cx,0,cz});return c?c->get(floorMod(x,Chunk::Width),y,floorMod(z,Chunk::Depth)):0;}
void World::markDirty(const ChunkPosition&p){dirty_.insert(p);}
bool World::setBlock(int x,int y,int z,BlockId b){if(y<0||y>=Chunk::Height)return false;int cx=floorDiv(x,Chunk::Width),cz=floorDiv(z,Chunk::Depth),lx=floorMod(x,Chunk::Width),lz=floorMod(z,Chunk::Depth);chunk({cx,0,cz}).set(lx,y,lz,b);markDirty({cx,0,cz});if(lx==0)markDirty({cx-1,0,cz});if(lx==Chunk::Width-1)markDirty({cx+1,0,cz});if(lz==0)markDirty({cx,0,cz-1});if(lz==Chunk::Depth-1)markDirty({cx,0,cz+1});return true;}
void World::generateChunk(int cx,int cz){Chunk c;TerrainGenerator(blocks_).generate(c,cx,cz);chunks_.insert_or_assign({cx,0,cz},std::move(c));markDirty({cx,0,cz});}
void World::generateTerrain(int radius){for(int z=-radius;z<=radius;++z)for(int x=-radius;x<=radius;++x)generateChunk(x,z);}
void World::updateStreaming(float px,float pz,int radius){int pcx=floorDiv((int)std::floor(px),Chunk::Width),pcz=floorDiv((int)std::floor(pz),Chunk::Depth);for(int z=pcz-radius;z<=pcz+radius;++z)for(int x=pcx-radius;x<=pcx+radius;++x)if(!findChunk({x,0,z}))generateChunk(x,z);std::vector<ChunkPosition> remove;for(auto&[p,c]:chunks_)if(std::abs(p.x-pcx)>radius+1||std::abs(p.z-pcz)>radius+1)remove.push_back(p);for(auto&p:remove){chunks_.erase(p);dirty_.erase(p);}}
}
