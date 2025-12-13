#pragma once

#include <algorithm>
#include <cstdint>
#include <random>
#include <type_traits>
#include <vector>

namespace poker_solver::util {

// Simple deterministic RNG wrapper around std::mt19937_64.
class Rng {
 public:
  explicit Rng(std::uint64_t seed) : engine_(seed) {}

  // Inclusive integer range.
  int UniformInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(engine_);
  }

  // Half-open real range [min, max).
  double UniformReal(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(engine_);
  }

  template <typename T>
  void Shuffle(std::vector<T>& values) {
    std::shuffle(values.begin(), values.end(), engine_);
  }

 private:
  std::mt19937_64 engine_;
};

}  // namespace poker_solver::util
