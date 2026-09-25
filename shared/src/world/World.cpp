#include "projectmc/world/World.hpp"
#include <cmath>
namespace projectmc::world {
World::World()=default;
int World::floorDiv(int v,int d){int q=v/d,r=v%d;if(r<0)--q;return q;}
int World::floorMod(int v,int d){int r=v%d;return r<0?r+d:r;}
Chunk& World::chunk(const ChunkPosition&p){return chunks_[p];}
const Chunk* World::findChunk(const ChunkPosition&p)const{auto it=chunks_.find(p);return it==chunks_.end()?nullptr:&it->second;}
BlockId World::getBlock(int x,int y,int z)const{
 if(y<0||y>=Chunk::Height)return 0;int cx=floorDiv(x,Chunk::Width),cz=floorDiv(z,Chunk::Depth);
 auto*c=findChunk({cx,0,cz});return c?c->get(floorMod(x,Chunk::Width),y,floorMod(z,Chunk::Depth)):0;
}
bool World::setBlock(int x,int y,int z,BlockId b){
 if(y<0||y>=Chunk::Height)return false;int cx=floorDiv(x,Chunk::Width),cz=floorDiv(z,Chunk::Depth);
 chunk({cx,0,cz}).set(floorMod(x,Chunk::Width),y,floorMod(z,Chunk::Depth),b);return true;
}
void World::generateChunk(int cx,int cz){
 auto&c=chunk({cx,0,cz});auto stone=blocks_.id("stone"),dirt=blocks_.id("dirt"),grass=blocks_.id("grass"),sand=blocks_.id("sand"),water=blocks_.id("water");
 for(int z=0;z<Chunk::Depth;++z)for(int x=0;x<Chunk::Width;++x){
  int wx=cx*Chunk::Width+x,wz=cz*Chunk::Depth+z;
  float hills=std::sin(wx*.115f)*1.8f+std::cos(wz*.095f)*1.5f+std::sin((wx+wz)*.047f)*1.2f;
  int surface=6+(int)std::round(hills);if(surface<2)surface=2;if(surface>12)surface=12;
  for(int y=0;y<surface-2;++y)c.set(x,y,z,stone);
  c.set(x,surface-2,z,dirt);c.set(x,surface-1,z,surface<=5?sand:grass);
  constexpr int sea=5;if(surface<sea)for(int y=surface;y<sea;++y)c.set(x,y,z,water);
 }
}
void World::generateTerrain(int radius){for(int z=-radius;z<=radius;++z)for(int x=-radius;x<=radius;++x)generateChunk(x,z);}
}
