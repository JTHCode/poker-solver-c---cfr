#pragma once

#include <algorithm>
#include <array>
#include <stdexcept>

#include "core/board.h"
#include "core/cards.h"
#include "core/hand_rank.h"

namespace poker_solver::core {

inline bool IsFlush(const std::array<Card, 5>& cards) {
  const Suit s = cards[0].suit;
  for (int i = 1; i < 5; ++i) {
    if (cards[i].suit != s) {
      return false;
    }
  }
  return true;
}

inline int StraightHighRank(std::array<int, 5> ranks_desc) {
  std::sort(ranks_desc.begin(), ranks_desc.end(), std::greater<int>());
  std::array<int, 5> uniq{};
  int n = 0;
  for (int i = 0; i < 5; ++i) {
    if (i == 0 || ranks_desc[i] != ranks_desc[i - 1]) {
      uniq[n++] = ranks_desc[i];
    }
  }
  if (n != 5) {
    return 0;
  }
  // Wheel: A-5-4-3-2
  if (uniq[0] == 14 && uniq[1] == 5 && uniq[2] == 4 && uniq[3] == 3 && uniq[4] == 2) {
    return 5;
  }
  for (int i = 0; i < 4; ++i) {
    if (uniq[i] != uniq[i + 1] + 1) {
      return 0;
    }
  }
  return uniq[0];
}

inline HandRank Evaluate5(const std::array<Card, 5>& cards) {
  std::array<int, 5> ranks{};
  for (int i = 0; i < 5; ++i) {
    ranks[i] = static_cast<int>(cards[i].rank);
  }

  std::array<int, 15> count{};
  count.fill(0);
  for (int r : ranks) {
    if (r < 2 || r > 14) {
      throw std::invalid_argument("Invalid rank");
    }
    count[static_cast<std::size_t>(r)] += 1;
  }

  const bool flush = IsFlush(cards);
  const int straight_high = StraightHighRank(ranks);

  // Collect groups: (count, rank) sorted by count desc then rank desc.
  std::array<std::pair<int, int>, 5> groups{};
  int g = 0;
  for (int r = 14; r >= 2; --r) {
    if (count[static_cast<std::size_t>(r)] > 0) {
      if (g >= 5) {
        throw std::logic_error("Too many distinct ranks in 5-card hand");
      }
      groups[static_cast<std::size_t>(g++)] = {count[static_cast<std::size_t>(r)], r};
    }
  }
  std::sort(groups.begin(), groups.end(), [](const auto& a, const auto& b) {
    if (a.first != b.first) {
      return a.first > b.first;
    }
    return a.second > b.second;
  });

  if (straight_high > 0 && flush) {
    return MakeHandRank(HandCategory::kStraightFlush, {straight_high, 0, 0, 0, 0});
  }

  if (groups[0].first == 4) {
    const int quad = groups[0].second;
    const int kicker = groups[1].second;
    return MakeHandRank(HandCategory::kQuads, {quad, kicker, 0, 0, 0});
  }

  if (groups[0].first == 3 && groups[1].first == 2) {
    return MakeHandRank(HandCategory::kFullHouse, {groups[0].second, groups[1].second, 0, 0, 0});
  }

  if (flush) {
    std::array<int, 5> sorted = ranks;
    std::sort(sorted.begin(), sorted.end(), std::greater<int>());
    return MakeHandRank(HandCategory::kFlush, sorted);
  }

  if (straight_high > 0) {
    return MakeHandRank(HandCategory::kStraight, {straight_high, 0, 0, 0, 0});
  }

  if (groups[0].first == 3) {
    const int trips = groups[0].second;
    std::array<int, 2> kick{};
    int kidx = 0;
    for (int i = 1; i < 5; ++i) {
      if (groups[i].first == 1) {
        if (kidx < 2) {
          kick[static_cast<std::size_t>(kidx++)] = groups[i].second;
        }
      }
    }
    return MakeHandRank(HandCategory::kTrips, {trips, kick[0], kick[1], 0, 0});
  }

  if (groups[0].first == 2 && groups[1].first == 2) {
    const int high_pair = std::max(groups[0].second, groups[1].second);
    const int low_pair = std::min(groups[0].second, groups[1].second);
    const int kicker = groups[2].second;
    return MakeHandRank(HandCategory::kTwoPair, {high_pair, low_pair, kicker, 0, 0});
  }

  if (groups[0].first == 2) {
    const int pair = groups[0].second;
    std::array<int, 3> kick{};
    int kidx = 0;
    for (int i = 1; i < 5; ++i) {
      if (groups[i].first == 1) {
        if (kidx < 3) {
          kick[static_cast<std::size_t>(kidx++)] = groups[i].second;
        }
      }
    }
    return MakeHandRank(HandCategory::kOnePair, {pair, kick[0], kick[1], kick[2], 0});
  }

  std::array<int, 5> sorted = ranks;
  std::sort(sorted.begin(), sorted.end(), std::greater<int>());
  return MakeHandRank(HandCategory::kHighCard, sorted);
}

inline HandRank EvaluateHoldem7(const std::array<Card, 2>& hole, const std::array<Card, 5>& board) {
  std::array<Card, 7> cards{hole[0], hole[1], board[0], board[1], board[2], board[3], board[4]};
  HandRank best{0};

  for (int a = 0; a < 7; ++a) {
    for (int b = a + 1; b < 7; ++b) {
      for (int c = b + 1; c < 7; ++c) {
        for (int d = c + 1; d < 7; ++d) {
          for (int e = d + 1; e < 7; ++e) {
            const std::array<Card, 5> five{cards[a], cards[b], cards[c], cards[d], cards[e]};
            const auto rank = Evaluate5(five);
            if (rank > best) {
              best = rank;
            }
          }
        }
      }
    }
  }
  return best;
}

inline HandRank EvaluateHoldem7(const std::array<Card, 2>& hole, const Board& board) {
  const std::array<Card, 5> b{board.flop[0], board.flop[1], board.flop[2], board.turn, board.river};
  return EvaluateHoldem7(hole, b);
}

}  // namespace poker_solver::core
