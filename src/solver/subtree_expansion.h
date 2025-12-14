#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "core/game_state.h"
#include "core/tree.h"
#include "solver/nlhe_cfr.h"

namespace poker_solver::solver {

struct RootStrategyResult {
  std::string info_set_key;
  std::vector<core::Action> actions;
  std::vector<double> probabilities;
  NlheCfrSolver::Stats stats;
};

struct BranchSolveResult {
  core::Action hero_root_action;
  double root_probability{0.0};
  std::vector<double> hero_root_avg_strategy_after_lock;
  NlheCfrSolver::Stats stats;
};

inline RootStrategyResult SolveRootStrategy(const core::GameState& root_state,
                                           const NlheCfrOptions& options) {
  if (root_state.current_player != 0) {
    throw std::invalid_argument("Phase-11 solver assumes hero is player0 at root");
  }
  NlheCfrSolver solver(options);
  solver.Solve(root_state);

  RootStrategyResult result;
  result.info_set_key = core::InfoSetKey(root_state, core::NodeOwner::kPlayer0);
  result.actions = solver.ActionsForInfoSet(root_state, core::NodeOwner::kPlayer0);
  result.probabilities = solver.AverageStrategy(root_state, core::NodeOwner::kPlayer0);
  result.stats = solver.stats();
  if (result.actions.size() != result.probabilities.size()) {
    throw std::logic_error("Root strategy action/probability size mismatch");
  }
  return result;
}

inline std::vector<std::size_t> SelectBranchesByThreshold(const std::vector<double>& probs,
                                                         double threshold) {
  if (threshold < 0.0 || threshold > 1.0) {
    throw std::invalid_argument("threshold must be in [0,1]");
  }
  std::vector<std::size_t> indices;
  for (std::size_t i = 0; i < probs.size(); ++i) {
    if (probs[i] >= threshold) {
      indices.push_back(i);
    }
  }
  return indices;
}

inline std::vector<BranchSolveResult> SolveBranches(const core::GameState& root_state,
                                                   const RootStrategyResult& root_strategy,
                                                   double threshold,
                                                   const NlheCfrOptions& branch_options) {
  if (root_strategy.actions.size() != root_strategy.probabilities.size()) {
    throw std::invalid_argument("RootStrategyResult is invalid");
  }

  const auto selected = SelectBranchesByThreshold(root_strategy.probabilities, threshold);
  std::vector<BranchSolveResult> results;
  results.reserve(selected.size());

  for (const auto idx : selected) {
    BranchSolveResult result;
    result.hero_root_action = root_strategy.actions[idx];
    result.root_probability = root_strategy.probabilities[idx];

    NlheCfrOptions opt = branch_options;
    opt.lock_root_action = result.hero_root_action;
    opt.villain_best_response = true;

    NlheCfrSolver solver(opt);
    solver.Solve(root_state);

    result.hero_root_avg_strategy_after_lock = solver.AverageStrategy(root_state, core::NodeOwner::kPlayer0);
    result.stats = solver.stats();
    results.push_back(std::move(result));
  }
  return results;
}

}  // namespace poker_solver::solver
