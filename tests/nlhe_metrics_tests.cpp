#include <cassert>

#include "core/cards.h"
#include "core/game_state.h"
#include "solver/nlhe_cfr.h"
#include "solver/nlhe_metrics.h"

namespace {

using poker_solver::core::Card;
using poker_solver::core::GameState;
using poker_solver::core::Rank;
using poker_solver::core::Street;
using poker_solver::core::Suit;
using poker_solver::solver::HoldemTerminalContext;
using poker_solver::solver::NlheCfrOptions;
using poker_solver::solver::NlheCfrSolver;
using poker_solver::solver::NlheExploitabilityRiverOnly;

GameState MakeRiverRootState() {
  GameState s{};
  s.street = Street::kRiver;

  // Public board: 2c 3d 4h 5s 7c
  s.board.flop[0] = Card{Rank::kTwo, Suit::kClubs};
  s.board.flop[1] = Card{Rank::kThree, Suit::kDiamonds};
  s.board.flop[2] = Card{Rank::kFour, Suit::kHearts};
  s.board.turn = Card{Rank::kFive, Suit::kSpades};
  s.board.river = Card{Rank::kSeven, Suit::kClubs};
  s.board_count = 5;

  s.pot = 20;
  s.committed[0] = 10;
  s.committed[1] = 10;
  s.stacks[0] = 100;
  s.stacks[1] = 100;

  s.to_call = 0;
  s.street_committed[0] = 0;
  s.street_committed[1] = 0;
  s.last_bet_size = 0;
  s.raises_this_street = 0;
  s.reopen_allowed = true;
  s.consecutive_checks = 0;
  s.current_player = 0;
  s.terminal = false;
  s.street_complete = false;
  s.winner = -1;
  return s;
}

HoldemTerminalContext MakeHoldemContext(const GameState& s) {
  HoldemTerminalContext ctx;
  // Player0: AsAh, Player1: KcKd. Player0 always wins on this board.
  ctx.p0_hole = {Card{Rank::kAce, Suit::kSpades}, Card{Rank::kAce, Suit::kHearts}};
  ctx.p1_hole = {Card{Rank::kKing, Suit::kClubs}, Card{Rank::kKing, Suit::kDiamonds}};
  ctx.board = s.board;
  ctx.board_count = 5;
  ctx.board_samples = 0;
  ctx.runout_seed = 123;
  poker_solver::solver::PrepareHoldemContext(ctx);
  return ctx;
}

void TestRiverExploitabilityDecreasesWithIterations() {
  const auto root = MakeRiverRootState();

  NlheCfrOptions opt;
  opt.iterations = 50;
  opt.showdown_winner = 0;
  opt.holdem = MakeHoldemContext(root);

  NlheCfrSolver a(opt);
  a.Solve(root);
  const auto ea = NlheExploitabilityRiverOnly(root, a, opt);

  NlheCfrOptions opt2 = opt;
  opt2.iterations = 400;
  NlheCfrSolver b(opt2);
  b.Solve(root);
  const auto eb = NlheExploitabilityRiverOnly(root, b, opt2);

  assert(ea.exploitability >= 0.0);
  assert(eb.exploitability >= 0.0);
  assert(eb.exploitability <= ea.exploitability + 1e-9);
}

}  // namespace

int main() {
  TestRiverExploitabilityDecreasesWithIterations();
  return 0;
}
