#include <cassert>
#include <string>

#include "core/game_state.h"
#include "core/tree.h"

namespace {

using poker_solver::core::GameState;
using poker_solver::core::InfoSetKey;
using poker_solver::core::NodeOwner;

void TestInfoSetDeterministic() {
  GameState state{};
  state.street = poker_solver::core::Street::kFlop;
  state.board.flop[0] = poker_solver::core::Card{poker_solver::core::Rank::kTwo, poker_solver::core::Suit::kClubs};
  state.board.flop[1] =
      poker_solver::core::Card{poker_solver::core::Rank::kThree, poker_solver::core::Suit::kDiamonds};
  state.board.flop[2] =
      poker_solver::core::Card{poker_solver::core::Rank::kFour, poker_solver::core::Suit::kHearts};
  state.board_count = 3;
  state.pot = 100;
  state.to_call = 20;
  state.last_bet_size = 20;
  state.street_committed[0] = 20;
  state.street_committed[1] = 0;
  state.current_player = 1;
  const auto key1 = InfoSetKey(state, NodeOwner::kPlayer1);
  const auto key2 = InfoSetKey(state, NodeOwner::kPlayer1);
  assert(key1 == key2);
}

void TestInfoSetDiffersByOwner() {
  GameState state{};
  state.pot = 50;
  const auto key_p0 = InfoSetKey(state, NodeOwner::kPlayer0);
  const auto key_p1 = InfoSetKey(state, NodeOwner::kPlayer1);
  assert(key_p0 != key_p1);
}

void TestTerminalFlagInKey() {
  GameState state{};
  state.pot = 10;
  const auto k1 = InfoSetKey(state, NodeOwner::kPlayer0);
  state.terminal = true;
  const auto k2 = InfoSetKey(state, NodeOwner::kPlayer0);
  assert(k1 != k2);
}

void TestInfoSetDiffersByPublicBoard() {
  GameState a{};
  a.street = poker_solver::core::Street::kFlop;
  a.pot = 10;
  a.board.flop[0] = poker_solver::core::Card{poker_solver::core::Rank::kTwo, poker_solver::core::Suit::kClubs};
  a.board.flop[1] =
      poker_solver::core::Card{poker_solver::core::Rank::kThree, poker_solver::core::Suit::kDiamonds};
  a.board.flop[2] =
      poker_solver::core::Card{poker_solver::core::Rank::kFour, poker_solver::core::Suit::kHearts};
  a.board_count = 3;

  GameState b = a;
  b.board.flop[0] = poker_solver::core::Card{poker_solver::core::Rank::kFive, poker_solver::core::Suit::kClubs};

  const auto ka = InfoSetKey(a, NodeOwner::kPlayer0);
  const auto kb = InfoSetKey(b, NodeOwner::kPlayer0);
  assert(ka != kb);
}

}  // namespace

int main() {
  TestInfoSetDeterministic();
  TestInfoSetDiffersByOwner();
  TestTerminalFlagInKey();
  TestInfoSetDiffersByPublicBoard();
  return 0;
}
