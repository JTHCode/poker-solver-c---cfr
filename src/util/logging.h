#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>

namespace poker_solver::util {

enum class LogLevel { kInfo, kWarn, kError };

inline const char* ToString(LogLevel level) {
  switch (level) {
    case LogLevel::kInfo:
      return "[INFO]";
    case LogLevel::kWarn:
      return "[WARN]";
    case LogLevel::kError:
      return "[ERROR]";
    default:
      return "[LOG]";
  }
}

inline void Log(LogLevel level, std::string_view message) {
  const auto now = std::chrono::system_clock::now();
  const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm tm {};
#if defined(_MSC_VER)
  localtime_s(&tm, &now_time);
#else
  localtime_r(&now_time, &tm);
#endif

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

  std::cerr << oss.str() << " " << ToString(level) << " " << message << std::endl;
}

}  // namespace poker_solver::util

#define LOG_INFO(msg) ::poker_solver::util::Log(::poker_solver::util::LogLevel::kInfo, msg)
#define LOG_WARN(msg) ::poker_solver::util::Log(::poker_solver::util::LogLevel::kWarn, msg)
#define LOG_ERROR(msg) ::poker_solver::util::Log(::poker_solver::util::LogLevel::kError, msg)
