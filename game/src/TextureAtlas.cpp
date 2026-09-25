#include "projectmc/game/TextureAtlas.hpp"
#include <array>
namespace projectmc::game {
TextureAtlas::~TextureAtlas(){if(texture_)SDL_DestroyTexture(texture_);}
bool TextureAtlas::create(SDL_Renderer*r){
 constexpr int tile=16,cols=6,w=tile*cols,h=tile;
 texture_=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STREAMING,w,h);if(!texture_)return false;
 std::array<unsigned,6> colors={0x77777fff,0x795438ff,0x5c9c45ff,0x648c49ff,0xd5c58aff,0x3c78d2c0};
 void*p=nullptr;int pitch=0;if(!SDL_LockTexture(texture_,nullptr,&p,&pitch))return false;
 for(int y=0;y<h;++y)for(int x=0;x<w;++x){int t=x/tile;unsigned base=colors[t];int checker=((x+y)&3)==0?12:0;auto*px=(unsigned*)((char*)p+y*pitch+x*4);unsigned R=((base>>24)&255),G=((base>>16)&255),B=((base>>8)&255),A=base&255;R=R+checker>255?255:R+checker;G=G+checker>255?255:G+checker;B=B+checker>255?255:B+checker;*px=(R<<24)|(G<<16)|(B<<8)|A;}
 SDL_UnlockTexture(texture_);SDL_SetTextureScaleMode(texture_,SDL_SCALEMODE_NEAREST);
 const char*n[6]={"stone","dirt","grass_top","grass_side","sand","water"};for(int i=0;i<6;++i)regions_[n[i]]={(float)(i*tile)/w,0,(float)((i+1)*tile)/w,1};return true;
}
AtlasRegion TextureAtlas::region(std::string_view n)const{auto it=regions_.find(std::string(n));return it==regions_.end()?AtlasRegion{}:it->second;}
}
