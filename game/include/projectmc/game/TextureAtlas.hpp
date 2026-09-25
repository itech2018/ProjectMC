#pragma once
#include <SDL3/SDL.h>
#include <string_view>
#include <unordered_map>
#include <string>
namespace projectmc::game {
struct AtlasRegion { float u0{},v0{},u1{},v1{}; };
class TextureAtlas {
public:
 bool create(SDL_Renderer* renderer);
 ~TextureAtlas();
 [[nodiscard]] SDL_Texture* texture() const noexcept{return texture_;}
 [[nodiscard]] AtlasRegion region(std::string_view name) const;
private:
 SDL_Texture* texture_{nullptr};
 std::unordered_map<std::string,AtlasRegion> regions_;
};
}
