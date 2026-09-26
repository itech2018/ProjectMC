#pragma once
#include <SDL3/SDL.h>
#include <string_view>
#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>
namespace projectmc::game {
struct AtlasRegion { float u0{},v0{},u1{},v1{}; };
class TextureAtlas {
public:
 bool create(SDL_Renderer* renderer);
 bool createCpu();
 ~TextureAtlas();
 [[nodiscard]] SDL_Texture* texture() const noexcept{return texture_;}
 [[nodiscard]] const std::vector<std::uint8_t>& pixels() const noexcept{return pixels_;}
 [[nodiscard]] int width() const noexcept{return width_;}
 [[nodiscard]] int height() const noexcept{return height_;}
 [[nodiscard]] AtlasRegion region(std::string_view name) const;
private:
 SDL_Texture* texture_{nullptr};
 std::vector<std::uint8_t> pixels_;
 int width_{0};
 int height_{0};
 std::unordered_map<std::string,AtlasRegion> regions_;
};
}
