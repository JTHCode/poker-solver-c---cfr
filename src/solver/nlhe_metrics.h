#pragma once

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/game_state.h"
#include "core/tree.h"
#include "solver/nlhe_cfr.h"
#include "solver/terminal_utility.h"

namespace poker_solver::solver {

struct NlheExploitabilityResult {
  double value_p0{0.0};
  double br0_value_p0{0.0};
  double br1_value_p0{0.0};
  double exploitability{0.0};
};

namespace detail {

inline double TerminalUtilityP0(const core::GameState& terminal_state, const NlheCfrOptions& options,
                                std::unordered_map<std::uint64_t, double>& runout_cache) {
  if (!terminal_state.terminal) {
    throw std::logic_error("TerminalUtilityP0 called on non-terminal state");
  }
  if (terminal_state.winner != -1) {
    return FoldUtilityP0(terminal_state.winner, terminal_state.committed[0], terminal_state.committed[1]);
  }

  if (options.holdem.has_value()) {
    HoldemTerminalContext ctx = *options.holdem;
    ctx.board = terminal_state.board;
    ctx.board_count = terminal_state.board_count;
    RunoutStats stats;
    return ExpectedShowdownUtilityP0(ctx, terminal_state.committed[0], terminal_state.committed[1], runout_cache, stats);
  }

  return FoldUtilityP0(options.showdown_winner, terminal_state.committed[0], terminal_state.committed[1]);
}

inline void LoadStrategyAt(const NlheCfrSolver& solver, const core::GameState& state, core::NodeOwner owner,
                           std::vector<core::Action>& actions_out, std::vector<double>& probs_out) {
  const std::string key = core::InfoSetKey(state, owner);
  actions_out = solver.ActionsForInfoSet(state, owner);
  if (actions_out.empty()) {
    actions_out = core::LegalActions(state).actions;
  }
  probs_out = solver.AverageStrategy(state, owner);
  if (probs_out.empty()) {
    probs_out.assign(actions_out.size(), 0.0);
    const double u = 1.0 / static_cast<double>(actions_out.size());
    std::fill(probs_out.begin(), probs_out.end(), u);
  }
  if (probs_out.size() != actions_out.size()) {
    throw std::logic_error("Strategy/action size mismatch at: " + key);
  }
  double sum = 0.0;
  for (double p : probs_out) {
    sum += p;
  }
  if (sum <= 0.0) {
    const double u = 1.0 / static_cast<double>(actions_out.size());
    std::fill(probs_out.begin(), probs_out.end(), u);
    return;
  }
  for (double& p : probs_out) {
    p = std::max(0.0, p) / sum;
  }
}

inline double EvalProfile(const core::GameState& state, const NlheCfrSolver& solver, const NlheCfrOptions& options,
                          std::unordered_map<std::uint64_t, double>& runout_cache,
                          std::unordered_map<std::string, double>& memo) {
  core::GameState s = state;
  if (!s.terminal && s.street_complete) {
    s.terminal = true;
    s.winner = -1;
  }
  if (s.terminal) {
    return TerminalUtilityP0(s, options, runout_cache);
  }

  const int player = s.current_player;
  const auto owner = (player == 0) ? core::NodeOwner::kPlayer0 : core::NodeOwner::kPlayer1;
  const std::string key = core::InfoSetKey(s, owner);
  if (const auto it = memo.find(key); it != memo.end()) {
    return it->second;
  }

  std::vector<core::Action> actions;
  std::vector<double> probs;
  LoadStrategyAt(solver, s, owner, actions, probs);

  double value = 0.0;
  for (std::size_t i = 0; i < actions.size(); ++i) {
    core::GameState next = s;
    core::ApplyAction(next, actions[i]);
    value += probs[i] * EvalProfile(next, solver, options, runout_cache, memo);
  }
  memo.emplace(key, value);
  return value;
}

inline double BestResponseValueP0(const core::GameState& state, const NlheCfrSolver& solver, const NlheCfrOptions& options,
                                  int br_player, std::unordered_map<std::uint64_t, double>& runout_cache,
                                  std::unordered_map<std::string, double>& memo) {
  core::GameState s = state;
  if (!s.terminal && s.street_complete) {
    s.terminal = true;
    s.winner = -1;
  }
  if (s.terminal) {
    return TerminalUtilityP0(s, options, runout_cache);
  }

  const int player = s.current_player;
  const auto owner = (player == 0) ? core::NodeOwner::kPlayer0 : core::NodeOwner::kPlayer1;
  std::string key = core::InfoSetKey(s, owner);
  key = std::to_string(br_player) + "|" + key;
  if (const auto it = memo.find(key); it != memo.end()) {
    return it->second;
  }

  const auto legal = core::LegalActions(s).actions;
  if (legal.empty()) {
    throw std::logic_error("Non-terminal state with no legal actions");
  }

  double value = 0.0;
  if (player == br_player) {
    value = (br_player == 0) ? -1e300 : 1e300;
    for (const auto& action : legal) {
      core::GameState next = s;
      core::ApplyAction(next, action);
      const double child = BestResponseValueP0(next, solver, options, br_player, runout_cache, memo);
      if (br_player == 0) {
        value = std::max(value, child);
      } else {
        value = std::min(value, child);
      }
    }
  } else {
    std::vector<core::Action> actions;
    std::vector<double> probs;
    LoadStrategyAt(solver, s, owner, actions, probs);
    for (std::size_t i = 0; i < actions.size(); ++i) {
      core::GameState next = s;
      core::ApplyAction(next, actions[i]);
      value += probs[i] * BestResponseValueP0(next, solver, options, br_player, runout_cache, memo);
    }
  }

  memo.emplace(key, value);
  return value;
}

}  // namespace detail

inline NlheExploitabilityResult NlheExploitabilityRiverOnly(const core::GameState& root_state,
                                                           const NlheCfrSolver& solver,
                                                           const NlheCfrOptions& options) {
  if (root_state.street != core::Street::kRiver) {
    throw std::invalid_argument("NlheExploitabilityRiverOnly requires a river root_state");
  }

  std::unordered_map<std::uint64_t, double> runout_cache;

  std::unordered_map<std::string, double> memo_v;
  std::unordered_map<std::string, double> memo_br0;
  std::unordered_map<std::string, double> memo_br1;

  const double v = detail::EvalProfile(root_state, solver, options, runout_cache, memo_v);
  const double br0 = detail::BestResponseValueP0(root_state, solver, options, 0, runout_cache, memo_br0);
  const double br1 = detail::BestResponseValueP0(root_state, solver, options, 1, runout_cache, memo_br1);

  NlheExploitabilityResult out;
  out.value_p0 = v;
  out.br0_value_p0 = br0;
  out.br1_value_p0 = br1;
  out.exploitability = 0.5 * (br0 - br1);
  if (!std::isfinite(out.exploitability)) {
    throw std::logic_error("Non-finite NLHE exploitability computed");
  }
  return out;
}

}  // namespace poker_solver::solver
