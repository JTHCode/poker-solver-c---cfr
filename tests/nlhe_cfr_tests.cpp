#include <algorithm>
#include <cassert>
#include <vector>

#include "core/game_state.h"
#include "core/tree.h"
#include "solver/nlhe_cfr.h"

namespace {

using poker_solver::core::ActionType;
using poker_solver::core::GameState;
using poker_solver::core::NodeOwner;
using poker_solver::solver::NlheCfrOptions;
using poker_solver::solver::NlheCfrSolver;

GameState MakeRiverFacingBetState() {
  GameState state{};
  state.street = poker_solver::core::Street::kRiver;

  // Simple "ante" model: each player has committed 1 already.
  state.committed[0] = 1;
  state.committed[1] = 1;
  state.pot = 2;

  // Keep player0 stack equal to to_call so no raises are legal at the root (fold/call only).
  state.stacks[0] = 1;
  state.stacks[1] = 2;

  // Player 0 facing a 1-chip bet (to_call=1), last_bet_size=1.
  state.to_call = 1;
  state.last_bet_size = 1;
  state.street_committed[0] = 0;
  state.street_committed[1] = 1;
  state.current_player = 0;
  return state;
}

void TestFoldDominatesWhenLosingShowdown() {
  const auto root = MakeRiverFacingBetState();
  NlheCfrOptions options;
  options.iterations = 8000;
  options.showdown_winner = 1;
  NlheCfrSolver solver(options);
  solver.Solve(root);

  const std::string key = poker_solver::core::InfoSetKey(root, NodeOwner::kPlayer0);
  const auto actions = solver.ActionsForInfoSet(key);
  const auto avg = solver.AverageStrategy(key);

  assert(actions.size() == avg.size());
  assert(!actions.empty());
  assert(actions[0].type == ActionType::kFold);
  assert(actions[1].type == ActionType::kCall);

  // If player 0 always loses showdown, folding should dominate calling/raising.
  assert(avg[0] > 0.85);
}

void TestCallDominatesWhenWinningShowdown() {
  const auto root = MakeRiverFacingBetState();
  NlheCfrOptions options;
  options.iterations = 8000;
  options.showdown_winner = 0;
  NlheCfrSolver solver(options);
  solver.Solve(root);

  const std::string key = poker_solver::core::InfoSetKey(root, NodeOwner::kPlayer0);
  const auto actions = solver.ActionsForInfoSet(key);
  const auto avg = solver.AverageStrategy(key);

  assert(actions.size() == avg.size());
  assert(actions[0].type == ActionType::kFold);
  assert(actions[1].type == ActionType::kCall);

  // If player 0 always wins showdown, calling should dominate folding.
  assert(avg[1] > 0.85);
}

}  // namespace

int main() {
  TestFoldDominatesWhenLosingShowdown();
  TestCallDominatesWhenWinningShowdown();
  return 0;
}
