#include "projectmc/game/TextureAtlas.hpp"
#include <array>
#include <cstring>

namespace projectmc::game {

TextureAtlas::~TextureAtlas() {
 if(texture_) SDL_DestroyTexture(texture_);
}

bool TextureAtlas::createCpu() {
 constexpr int tile=16;
 constexpr int cols=6;
 width_=tile*cols;
 height_=tile;
 pixels_.resize(static_cast<std::size_t>(width_)*height_*4);

 const std::array<unsigned,6> colors={
  0x77777fff,0x795438ff,0x5c9c45ff,0x648c49ff,0xd5c58aff,0x3c78d2c0
 };

 for(int y=0;y<height_;++y) {
  for(int x=0;x<width_;++x) {
   const int t=x/tile;
   const unsigned base=colors[t];
   const int checker=((x+y)&3)==0?12:0;
   auto* px=&pixels_[(static_cast<std::size_t>(y)*width_+x)*4];
   unsigned r=(base>>24)&255,g=(base>>16)&255,b=(base>>8)&255,a=base&255;
   r=r+checker>255?255:r+checker;
   g=g+checker>255?255:g+checker;
   b=b+checker>255?255:b+checker;
   px[0]=static_cast<std::uint8_t>(r);
   px[1]=static_cast<std::uint8_t>(g);
   px[2]=static_cast<std::uint8_t>(b);
   px[3]=static_cast<std::uint8_t>(a);
  }
 }

 const char* names[6]={"stone","dirt","grass_top","grass_side","sand","water"};
 for(int i=0;i<6;++i)
  regions_[names[i]]={static_cast<float>(i*tile)/width_,0.0f,static_cast<float>((i+1)*tile)/width_,1.0f};
 return true;
}

bool TextureAtlas::create(SDL_Renderer* renderer) {
 if(!createCpu()||!renderer) return false;
 texture_=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STATIC,width_,height_);
 if(!texture_) return false;
 if(!SDL_UpdateTexture(texture_,nullptr,pixels_.data(),width_*4)) return false;
 SDL_SetTextureScaleMode(texture_,SDL_SCALEMODE_NEAREST);
 return true;
}

AtlasRegion TextureAtlas::region(std::string_view name) const {
 auto it=regions_.find(std::string(name));
 return it==regions_.end()?AtlasRegion{}:it->second;
}

}
