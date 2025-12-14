#pragma once

#include <array>
#include <cstdint>

namespace poker_solver::core {

enum class HandCategory : std::uint8_t {
  kHighCard = 0,
  kOnePair = 1,
  kTwoPair = 2,
  kTrips = 3,
  kStraight = 4,
  kFlush = 5,
  kFullHouse = 6,
  kQuads = 7,
  kStraightFlush = 8,
};

struct HandRank {
  // Higher is better.
  std::uint64_t value{0};
};

inline bool operator<(const HandRank& a, const HandRank& b) { return a.value < b.value; }
inline bool operator>(const HandRank& a, const HandRank& b) { return a.value > b.value; }
inline bool operator==(const HandRank& a, const HandRank& b) { return a.value == b.value; }
inline bool operator!=(const HandRank& a, const HandRank& b) { return !(a == b); }

inline HandRank MakeHandRank(HandCategory category, const std::array<int, 5>& kickers) {
  // Pack into: [category:4 bits][k1:4][k2:4][k3:4][k4:4][k5:4]
  std::uint64_t v = 0;
  v |= (static_cast<std::uint64_t>(category) & 0xF) << 20;
  v |= (static_cast<std::uint64_t>(kickers[0]) & 0xF) << 16;
  v |= (static_cast<std::uint64_t>(kickers[1]) & 0xF) << 12;
  v |= (static_cast<std::uint64_t>(kickers[2]) & 0xF) << 8;
  v |= (static_cast<std::uint64_t>(kickers[3]) & 0xF) << 4;
  v |= (static_cast<std::uint64_t>(kickers[4]) & 0xF);
  return HandRank{v};
}

}  // namespace poker_solver::core
