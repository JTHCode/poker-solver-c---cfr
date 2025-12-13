#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace poker_solver::core {

enum class Suit : std::uint8_t { kClubs = 0, kDiamonds = 1, kHearts = 2, kSpades = 3 };

// Rank values: 2..14 (Ace high as 14).
enum class Rank : std::uint8_t {
  kTwo = 2,
  kThree,
  kFour,
  kFive,
  kSix,
  kSeven,
  kEight,
  kNine,
  kTen,
  kJack,
  kQueen,
  kKing,
  kAce
};

struct Card {
  Rank rank;
  Suit suit;
};

inline bool operator==(const Card& a, const Card& b) {
  return a.rank == b.rank && a.suit == b.suit;
}

inline bool operator!=(const Card& a, const Card& b) { return !(a == b); }

// Converts a card to a 0..51 id: rank_index * 4 + suit.
inline std::uint8_t ToId(const Card& card) {
  const int rank_index = static_cast<int>(card.rank) - 2;  // 0-based for deuce.
  return static_cast<std::uint8_t>(rank_index * 4 + static_cast<int>(card.suit));
}

inline Card FromId(std::uint8_t id) {
  if (id >= 52) {
    throw std::out_of_range("Card id must be in [0,51]");
  }
  const int rank_index = id / 4;
  const int suit_index = id % 4;
  return Card{static_cast<Rank>(rank_index + 2), static_cast<Suit>(suit_index)};
}

inline std::string ToString(const Card& card) {
  static constexpr std::array<const char*, 15> kRankStr{
      "", "", "2", "3", "4", "5", "6", "7", "8", "9", "T", "J", "Q", "K", "A"};
  static constexpr std::array<const char*, 4> kSuitStr{"c", "d", "h", "s"};
  return std::string(kRankStr[static_cast<int>(card.rank)]) + kSuitStr[static_cast<int>(card.suit)];
}

inline std::vector<Card> StandardDeck() {
  std::vector<Card> deck;
  deck.reserve(52);
  for (int r = static_cast<int>(Rank::kTwo); r <= static_cast<int>(Rank::kAce); ++r) {
    for (int s = 0; s < 4; ++s) {
      deck.push_back(Card{static_cast<Rank>(r), static_cast<Suit>(s)});
    }
  }
  return deck;
}

}  // namespace poker_solver::core
