#include <cassert>
#include <unordered_map>

#include "core/cards.h"
#include "core/hand_evaluator.h"
#include "solver/terminal_utility.h"

namespace {

using poker_solver::core::Card;
using poker_solver::core::Evaluate5;
using poker_solver::core::Rank;
using poker_solver::core::Suit;
using poker_solver::core::HandCategory;
using poker_solver::solver::ExpectedShowdownUtilityP0;
using poker_solver::solver::FoldUtilityP0;
using poker_solver::solver::HoldemTerminalContext;
using poker_solver::solver::RunoutStats;
using poker_solver::solver::ShowdownUtilityP0;

int Category(const poker_solver::core::HandRank& r) { return static_cast<int>((r.value >> 20) & 0xF); }
int Kicker0(const poker_solver::core::HandRank& r) { return static_cast<int>((r.value >> 16) & 0xF); }

Card C(Rank r, Suit s) { return Card{r, s}; }

void TestEvaluate5StraightFlushAndWheel() {
  const auto sf = Evaluate5({C(Rank::kAce, Suit::kSpades),
                             C(Rank::kKing, Suit::kSpades),
                             C(Rank::kQueen, Suit::kSpades),
                             C(Rank::kJack, Suit::kSpades),
                             C(Rank::kTen, Suit::kSpades)});
  assert(Category(sf) == static_cast<int>(HandCategory::kStraightFlush));
  assert(Kicker0(sf) == 14);

  const auto wheel =
      Evaluate5({C(Rank::kAce, Suit::kClubs), C(Rank::kTwo, Suit::kDiamonds), C(Rank::kThree, Suit::kHearts),
                 C(Rank::kFour, Suit::kSpades), C(Rank::kFive, Suit::kClubs)});
  assert(Category(wheel) == static_cast<int>(HandCategory::kStraight));
  assert(Kicker0(wheel) == 5);
}

void TestShowdownTieUtility() {
  poker_solver::core::Board board;
  board.flop[0] = C(Rank::kTwo, Suit::kClubs);
  board.flop[1] = C(Rank::kThree, Suit::kDiamonds);
  board.flop[2] = C(Rank::kFour, Suit::kHearts);
  board.turn = C(Rank::kFive, Suit::kSpades);
  board.river = C(Rank::kSix, Suit::kClubs);

  const std::array<Card, 2> p0{C(Rank::kAce, Suit::kSpades), C(Rank::kKing, Suit::kDiamonds)};
  const std::array<Card, 2> p1{C(Rank::kQueen, Suit::kHearts), C(Rank::kJack, Suit::kDiamonds)};

  const double util = ShowdownUtilityP0(p0, p1, board, 10, 14);
  assert(util == 2.0);
}

void TestFoldUtility() {
  assert(FoldUtilityP0(0, 10, 7) == 7.0);
  assert(FoldUtilityP0(1, 10, 7) == -10.0);
}

void TestAllInRunoutExactAndSampling() {
  HoldemTerminalContext ctx;
  ctx.p0_hole = {C(Rank::kAce, Suit::kSpades), C(Rank::kKing, Suit::kSpades)};
  ctx.p1_hole = {C(Rank::kAce, Suit::kClubs), C(Rank::kKing, Suit::kClubs)};
  ctx.board.flop[0] = C(Rank::kQueen, Suit::kSpades);
  ctx.board.flop[1] = C(Rank::kJack, Suit::kSpades);
  ctx.board.flop[2] = C(Rank::kTen, Suit::kSpades);
  ctx.board.turn = C(Rank::kTwo, Suit::kDiamonds);   // placeholder
  ctx.board.river = C(Rank::kThree, Suit::kHearts);  // placeholder
  ctx.board_count = 3;
  ctx.board_samples = 0;

  std::unordered_map<std::uint64_t, double> cache;
  RunoutStats stats1;
  const double ev_exact = ExpectedShowdownUtilityP0(ctx, 10, 10, cache, stats1);
  assert(ev_exact == 10.0);
  assert(stats1.chance_samples == 990);  // C(45,2): remaining deck size is 52 - 4 hole - 3 board = 45

  RunoutStats stats_cache_hit;
  const double ev_cached = ExpectedShowdownUtilityP0(ctx, 10, 10, cache, stats_cache_hit);
  assert(ev_cached == 10.0);
  assert(stats_cache_hit.chance_samples == 0);

  HoldemTerminalContext sampled = ctx;
  sampled.board_samples = 100;
  sampled.runout_seed = 123;
  std::unordered_map<std::uint64_t, double> cache2;
  RunoutStats stats2;
  const double ev_sampled = ExpectedShowdownUtilityP0(sampled, 10, 10, cache2, stats2);
  assert(ev_sampled == 10.0);
  assert(stats2.chance_samples == 100);
}

}  // namespace

int main() {
  TestEvaluate5StraightFlushAndWheel();
  TestShowdownTieUtility();
  TestFoldUtility();
  TestAllInRunoutExactAndSampling();
  return 0;
}
