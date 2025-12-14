#include <algorithm>
#include <cassert>
#include <cmath>
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

void AssertValidDistribution(const std::vector<double>& probs) {
  assert(!probs.empty());
  double sum = 0.0;
  for (const double prob : probs) {
    assert(prob >= -1e-12);
    assert(prob <= 1.0 + 1e-12);
    sum += prob;
  }
  assert(std::abs(sum - 1.0) < 1e-6);
}

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

  const auto actions = solver.ActionsForInfoSet(root, NodeOwner::kPlayer0);
  const auto avg = solver.AverageStrategy(root, NodeOwner::kPlayer0);

  assert(actions.size() == avg.size());
  assert(!actions.empty());
  AssertValidDistribution(avg);
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

  const auto actions = solver.ActionsForInfoSet(root, NodeOwner::kPlayer0);
  const auto avg = solver.AverageStrategy(root, NodeOwner::kPlayer0);

  assert(actions.size() == avg.size());
  AssertValidDistribution(avg);
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
