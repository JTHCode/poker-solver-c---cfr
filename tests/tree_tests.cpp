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
  state.pot = 100;
  state.to_call = 20;
  state.last_bet_size = 20;
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

}  // namespace

int main() {
  TestInfoSetDeterministic();
  TestInfoSetDiffersByOwner();
  TestTerminalFlagInKey();
  return 0;
}
