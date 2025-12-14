#include <cassert>
#include <vector>

#include "core/game_state.h"
#include "solver/nlhe_cfr.h"
#include "solver/subtree_expansion.h"

namespace {

using poker_solver::core::Action;
using poker_solver::core::ActionType;
using poker_solver::core::GameState;
using poker_solver::core::Street;
using poker_solver::solver::NlheCfrOptions;
using poker_solver::solver::RootStrategyResult;
using poker_solver::solver::SelectBranchesByThreshold;
using poker_solver::solver::SolveBranches;
using poker_solver::solver::SolveRootStrategy;

GameState MakeRootStateToCallZero() {
  GameState s{};
  s.street = Street::kRiver;
  s.pot = 2;
  s.to_call = 0;
  s.stacks[0] = 3;
  s.stacks[1] = 3;
  s.committed[0] = 1;
  s.committed[1] = 1;
  s.current_player = 0;
  return s;
}

void TestThresholdSelection() {
  const std::vector<double> probs{0.19, 0.20, 0.21, 0.0, 1.0};
  const auto idx = SelectBranchesByThreshold(probs, 0.20);
  assert(idx.size() == 3);
  assert(idx[0] == 1);
  assert(idx[1] == 2);
  assert(idx[2] == 4);
}

void TestLockedRootActionEnforced() {
  const auto root = MakeRootStateToCallZero();

  NlheCfrOptions root_opt;
  root_opt.iterations = 2000;
  root_opt.showdown_winner = 0;
  const RootStrategyResult root_strat = SolveRootStrategy(root, root_opt);
  assert(!root_strat.actions.empty());

  // Force-choose a specific legal bet size at root (bet 1 if present), else take first action.
  Action chosen = root_strat.actions.front();
  for (const auto& a : root_strat.actions) {
    if (a.type == ActionType::kBet && a.amount == 1) {
      chosen = a;
      break;
    }
  }

  NlheCfrOptions branch_opt;
  branch_opt.iterations = 2000;
  branch_opt.showdown_winner = 0;

  RootStrategyResult synthetic = root_strat;
  synthetic.probabilities.assign(synthetic.actions.size(), 0.0);
  for (std::size_t i = 0; i < synthetic.actions.size(); ++i) {
    if (synthetic.actions[i] == chosen) {
      synthetic.probabilities[i] = 1.0;
      break;
    }
  }

  const auto branches = SolveBranches(root, synthetic, 0.20, branch_opt);
  assert(branches.size() == 1);
  assert(branches[0].hero_root_action == chosen);
  assert(!branches[0].hero_root_avg_strategy_after_lock.empty());

  // Locked action should have ~1.0 probability at root.
  double locked_prob = 0.0;
  for (std::size_t i = 0; i < root_strat.actions.size(); ++i) {
    if (root_strat.actions[i] == chosen) {
      locked_prob = branches[0].hero_root_avg_strategy_after_lock[i];
      break;
    }
  }
  assert(locked_prob > 0.999);
}

}  // namespace

int main() {
  TestThresholdSelection();
  TestLockedRootActionEnforced();
  return 0;
}

