#include "projectmc/Log.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
namespace projectmc {
void log(LogLevel level, std::string_view message) {
  const char* label = "INFO";
  switch (level) {
    case LogLevel::Debug: label = "DEBUG"; break;
    case LogLevel::Info: label = "INFO"; break;
    case LogLevel::Warning: label = "WARN"; break;
    case LogLevel::Error: label = "ERROR"; break;
  }
  const auto now = std::chrono::system_clock::now();
  const auto t = std::chrono::system_clock::to_time_t(now);
  std::cout << '[' << std::put_time(std::localtime(&t), "%H:%M:%S") << "] [" << label << "] " << message << '\n';
}
}
