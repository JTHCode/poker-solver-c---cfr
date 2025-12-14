#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/game_state.h"
#include "core/tree.h"
#include "solver/terminal_utility.h"

namespace poker_solver::solver {

struct NlheCfrOptions {
  int iterations{1000};
  int showdown_winner{0};  // 0 or 1 (used when state.winner is unresolved at showdown)
  bool villain_best_response{false};
  std::optional<core::Action> lock_root_action;  // if set, forces player0's root action
  std::optional<HoldemTerminalContext> holdem;   // if set, compute real showdown utilities
};

struct CfrNode {
  std::vector<core::Action> actions;
  std::vector<double> regret_sum;
  std::vector<double> strategy_sum;
};

class NlheCfrSolver {
 public:
  struct Stats {
    std::uint64_t nodes_visited{0};
    std::uint64_t decision_nodes{0};
    std::uint64_t terminal_evals{0};
    std::uint64_t legal_actions_total{0};
    std::uint64_t chance_samples{0};
  };

  explicit NlheCfrSolver(NlheCfrOptions options) : options_(options) {
    if (options_.iterations <= 0) {
      throw std::invalid_argument("iterations must be positive");
    }
    if (options_.showdown_winner != 0 && options_.showdown_winner != 1) {
      throw std::invalid_argument("showdown_winner must be 0 or 1");
    }
  }

  void Solve(const core::GameState& root_state) {
    root_state_ = root_state;
    root_key_ = core::InfoSetKey(root_state, core::NodeOwner::kPlayer0);
    stats_ = Stats{};
    runout_cache_.clear();
    for (int i = 0; i < options_.iterations; ++i) {
      (void)Cfr(root_state, 1.0, 1.0);
    }
  }

  std::vector<double> AverageStrategy(const std::string& info_set_key) const {
    const auto it = nodes_.find(info_set_key);
    if (it == nodes_.end() || it->second.strategy_sum.empty()) {
      return {};
    }
    const auto& sum = it->second.strategy_sum;
    double total = 0.0;
    for (double v : sum) {
      total += v;
    }
    if (total <= 0.0) {
      return std::vector<double>(sum.size(), 1.0 / static_cast<double>(sum.size()));
    }
    std::vector<double> avg(sum.size());
    for (std::size_t i = 0; i < sum.size(); ++i) {
      avg[i] = sum[i] / total;
    }
    return avg;
  }

  std::vector<core::Action> ActionsForInfoSet(const std::string& info_set_key) const {
    const auto it = nodes_.find(info_set_key);
    if (it == nodes_.end()) {
      return {};
    }
    return it->second.actions;
  }

  const Stats& stats() const { return stats_; }

 private:
  static std::vector<double> RegretMatching(const std::vector<double>& regret_sum) {
    std::vector<double> strategy(regret_sum.size(), 0.0);
    double sum_positive = 0.0;
    for (double r : regret_sum) {
      if (r > 0.0) {
        sum_positive += r;
      }
    }
    if (sum_positive > 0.0) {
      for (std::size_t i = 0; i < regret_sum.size(); ++i) {
        strategy[i] = (regret_sum[i] > 0.0) ? (regret_sum[i] / sum_positive) : 0.0;
      }
      return strategy;
    }
    const double uniform = 1.0 / static_cast<double>(regret_sum.size());
    std::fill(strategy.begin(), strategy.end(), uniform);
    return strategy;
  }

  double TerminalUtility(const core::GameState& state) const {
    if (!state.terminal) {
      throw std::logic_error("TerminalUtility called on non-terminal state");
    }

    if (state.winner != -1) {
      return FoldUtilityP0(state.winner, state.committed[0], state.committed[1]);
    }

    if (options_.holdem.has_value()) {
      RunoutStats local;
      const double ev = ExpectedShowdownUtilityP0(*options_.holdem, state.committed[0], state.committed[1],
                                                  runout_cache_, local);
      stats_.chance_samples += local.chance_samples;
      return ev;
    }

    // Fallback stub until evaluation context is provided.
    const int winner = options_.showdown_winner;
    return FoldUtilityP0(winner, state.committed[0], state.committed[1]);
  }

  double Cfr(const core::GameState& state, double p0_reach, double p1_reach) {
    ++stats_.nodes_visited;
    if (state.terminal) {
      ++stats_.terminal_evals;
      return TerminalUtility(state);
    }
    if (state.street_complete) {
      // For now, treat street completion as terminal showdown for single-street abstractions.
      core::GameState terminal_state = state;
      terminal_state.terminal = true;
      terminal_state.winner = -1;
      ++stats_.terminal_evals;
      return TerminalUtility(terminal_state);
    }

    ++stats_.decision_nodes;
    const int player = state.current_player;
    const auto owner = (player == 0) ? core::NodeOwner::kPlayer0 : core::NodeOwner::kPlayer1;
    const std::string key = core::InfoSetKey(state, owner);

    const auto legal = core::LegalActions(state).actions;
    if (legal.empty()) {
      throw std::logic_error("Non-terminal state with no legal actions");
    }
    stats_.legal_actions_total += static_cast<std::uint64_t>(legal.size());

    auto& node = nodes_[key];
    if (node.actions.empty()) {
      node.actions = legal;
      node.regret_sum.assign(legal.size(), 0.0);
      node.strategy_sum.assign(legal.size(), 0.0);
    } else if (node.actions.size() != legal.size()) {
      throw std::logic_error("Action count mismatch for infoset: " + key);
    }

    const bool is_root_lock = options_.lock_root_action.has_value() && player == 0 && key == root_key_;
    const bool villain_br = options_.villain_best_response && player == 1;

    std::vector<double> strategy;
    if (is_root_lock) {
      strategy.assign(legal.size(), 0.0);
      bool found = false;
      for (std::size_t i = 0; i < legal.size(); ++i) {
        if (legal[i] == *options_.lock_root_action) {
          strategy[i] = 1.0;
          found = true;
          break;
        }
      }
      if (!found) {
        throw std::invalid_argument("lock_root_action is not legal at root");
      }
    } else if (!villain_br) {
      strategy = RegretMatching(node.regret_sum);
    }

    std::vector<double> util(legal.size(), 0.0);
    double node_util = 0.0;
    for (std::size_t i = 0; i < legal.size(); ++i) {
      if (!strategy.empty() && strategy[i] == 0.0) {
        continue;
      }
      core::GameState next = state;
      core::ApplyAction(next, legal[i]);
      if (player == 0) {
        util[i] = Cfr(next, p0_reach * (strategy.empty() ? 1.0 : strategy[i]), p1_reach);
      } else {
        util[i] = Cfr(next, p0_reach, p1_reach * (strategy.empty() ? 1.0 : strategy[i]));
      }
      if (!strategy.empty()) {
        node_util += strategy[i] * util[i];
      }
    }

    if (villain_br) {
      // Villain minimizes player0 utility.
      std::size_t best = 0;
      double best_value = std::numeric_limits<double>::infinity();
      for (std::size_t i = 0; i < legal.size(); ++i) {
        if (util[i] < best_value) {
          best_value = util[i];
          best = i;
        }
      }
      strategy.assign(legal.size(), 0.0);
      strategy[best] = 1.0;
      node_util = util[best];
    }

    const double reach = (player == 0) ? p0_reach : p1_reach;
    for (std::size_t i = 0; i < node.strategy_sum.size(); ++i) {
      node.strategy_sum[i] += reach * strategy[i];
    }

    const double opp_reach = (player == 0) ? p1_reach : p0_reach;
    if (!villain_br && !is_root_lock) {
      for (std::size_t i = 0; i < legal.size(); ++i) {
        const double regret = (player == 0) ? (util[i] - node_util) : (node_util - util[i]);
        node.regret_sum[i] += opp_reach * regret;
      }
    }

    return node_util;
  }

  NlheCfrOptions options_;
  std::unordered_map<std::string, CfrNode> nodes_;
  core::GameState root_state_{};
  std::string root_key_;
  mutable Stats stats_{};
  mutable std::unordered_map<std::uint64_t, double> runout_cache_{};
};

}  // namespace poker_solver::solver
