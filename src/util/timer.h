#pragma once

#include <chrono>

namespace poker_solver::util {

// Lightweight wall-clock timer for measuring short durations.
class Timer {
 public:
  Timer() : start_(std::chrono::steady_clock::now()) {}

  void Reset() { start_ = std::chrono::steady_clock::now(); }

  // Returns elapsed milliseconds as double.
  double ElapsedMillis() const {
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration<double, std::milli>(now - start_);
    return elapsed.count();
  }

 private:
  std::chrono::steady_clock::time_point start_;
};

}  // namespace poker_solver::util
