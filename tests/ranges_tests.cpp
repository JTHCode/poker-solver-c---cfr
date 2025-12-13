#include <cassert>
#include <cmath>
#include <string>
#include <vector>

#include "core/cards.h"
#include "io/preflop_range.h"
#include "util/rng.h"

namespace {

using poker_solver::core::Card;
using poker_solver::core::Rank;
using poker_solver::core::Suit;
using poker_solver::io::LoadPreflopRange;
using poker_solver::io::SampleHand;
using poker_solver::util::Rng;

const std::string kRangePath =
    std::string(PROJECT_SOURCE_DIR) + "/preflop-ranges/100bb/open/UTG_OPEN.json";

void TestParseAndNormalize() {
  const auto range = LoadPreflopRange(kRangePath);

  assert(range.position == "UTG");
  assert(!range.villain_position.has_value());
  assert(range.situation == "OPEN");
  assert(range.stack == "100bb");
  assert(!range.combos.empty());

  double sum = 0.0;
  for (const auto& combo : range.combos) {
    sum += combo.weight;
    assert(combo.weight > 0.0);
  }
  assert(std::fabs(sum - 1.0) < 1e-6);
}

void TestSamplingRespectsBlockers() {
  const auto range = LoadPreflopRange(kRangePath);
  const std::vector<Card> blockers{
      Card{Rank::kAce, Suit::kSpades},
      Card{Rank::kKing, Suit::kSpades},
  };

  Rng rng(777);
  for (int i = 0; i < 200; ++i) {
    const auto hand = SampleHand(range, rng, blockers);
    for (const auto& blocked : blockers) {
      assert(hand.first != blocked);
      assert(hand.second != blocked);
    }
  }
}

}  // namespace

int main() {
  TestParseAndNormalize();
  TestSamplingRespectsBlockers();
  return 0;
}
