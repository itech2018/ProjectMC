#pragma once
#include <string_view>
namespace projectmc {
struct Version final {
  static constexpr int major = 0;
  static constexpr int minor = 0;
  static constexpr int patch = 1;
  static constexpr std::string_view stage = "dev";
  [[nodiscard]] static std::string_view string() noexcept;
};
}
