#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace poker_solver::util {

inline std::uint64_t Fnv1a64(std::string_view data) {
  constexpr std::uint64_t kOffset = 14695981039346656037ull;
  constexpr std::uint64_t kPrime = 1099511628211ull;
  std::uint64_t hash = kOffset;
  for (char c : data) {
    hash ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
    hash *= kPrime;
  }
  return hash;
}

inline std::string ToHex(std::uint64_t value) {
  static constexpr char kHex[] = "0123456789abcdef";
  std::string out(16, '0');
  for (int i = 15; i >= 0; --i) {
    out[static_cast<std::size_t>(i)] = kHex[value & 0xF];
    value >>= 4;
  }
  return out;
}

}  // namespace poker_solver::util
