#pragma once
#include <string_view>
namespace projectmc {
enum class LogLevel { Debug, Info, Warning, Error };
void log(LogLevel level, std::string_view message);
}
