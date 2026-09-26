#include "projectmc/world/TerrainGenerator.hpp"
#include <cmath>
namespace projectmc::world {
void TerrainGenerator::generate(Chunk&c,int cx,int cz)const{
 auto stone=blocks_.id("stone"),dirt=blocks_.id("dirt"),grass=blocks_.id("grass"),sand=blocks_.id("sand"),water=blocks_.id("water");
 for(int z=0;z<Chunk::Depth;++z)for(int x=0;x<Chunk::Width;++x){
  int wx=cx*Chunk::Width+x,wz=cz*Chunk::Depth+z;
  const float sx=static_cast<float>((seed_&0xffffu))*0.00037f;
  const float sz=static_cast<float>((seed_>>16)&0xffffu)*0.00041f;
  const float sm=static_cast<float>((seed_>>32)&0xffffu)*0.00029f;
  float hills=std::sin(wx*.115f+sx)*1.8f+std::cos(wz*.095f+sz)*1.5f+std::sin((wx+wz)*.047f+sm)*1.2f;
  int surface=6+(int)std::round(hills);if(surface<2)surface=2;if(surface>12)surface=12;
  for(int y=0;y<surface-2;++y)c.set(x,y,z,stone);
  c.set(x,surface-2,z,dirt);c.set(x,surface-1,z,surface<=5?sand:grass);
  constexpr int sea=5;if(surface<sea)for(int y=surface;y<sea;++y)c.set(x,y,z,water);
 }
}
}
