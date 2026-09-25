#include "projectmc/world/World.hpp"
namespace projectmc::world {
World::World()=default;
Chunk& World::chunk(const ChunkPosition& p){return chunks_[p];}
const Chunk* World::findChunk(const ChunkPosition& p) const {auto it=chunks_.find(p);return it==chunks_.end()?nullptr:&it->second;}
BlockId World::getBlock(int x,int y,int z) const {
 if(x<0||y<0||z<0||x>=Chunk::Width||y>=Chunk::Height||z>=Chunk::Depth)return 0;
 auto*c=findChunk({0,0,0});return c?c->get(x,y,z):0;
}
bool World::setBlock(int x,int y,int z,BlockId b){
 if(x<0||y<0||z<0||x>=Chunk::Width||y>=Chunk::Height||z>=Chunk::Depth)return false;
 chunk({0,0,0}).set(x,y,z,b);return true;
}
void World::generateTestWorld(){auto&c=chunk({0,0,0});auto stone=blocks_.id("stone"),dirt=blocks_.id("dirt"),grass=blocks_.id("grass");for(int z=0;z<Chunk::Depth;++z)for(int x=0;x<Chunk::Width;++x){for(int y=0;y<3;++y)c.set(x,y,z,stone);c.set(x,3,z,dirt);c.set(x,4,z,grass);}}
}
