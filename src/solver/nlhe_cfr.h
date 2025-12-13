#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/game_state.h"
#include "core/tree.h"

namespace poker_solver::solver {

struct NlheCfrOptions {
  int iterations{1000};
  int showdown_winner{0};  // 0 or 1 (used when state.winner is unresolved at showdown)
};

struct CfrNode {
  std::vector<core::Action> actions;
  std::vector<double> regret_sum;
  std::vector<double> strategy_sum;
};

class NlheCfrSolver {
 public:
  explicit NlheCfrSolver(NlheCfrOptions options) : options_(options) {
    if (options_.iterations <= 0) {
      throw std::invalid_argument("iterations must be positive");
    }
    if (options_.showdown_winner != 0 && options_.showdown_winner != 1) {
      throw std::invalid_argument("showdown_winner must be 0 or 1");
    }
  }

  void Solve(const core::GameState& root_state) {
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

    const int winner = (state.winner == -1) ? options_.showdown_winner : state.winner;
    if (winner == 0) {
      return static_cast<double>(state.committed[1]);
    }
    return -static_cast<double>(state.committed[0]);
  }

  double Cfr(const core::GameState& state, double p0_reach, double p1_reach) {
    if (state.terminal) {
      return TerminalUtility(state);
    }
    if (state.street_complete) {
      // For now, treat street completion as terminal showdown for single-street abstractions.
      core::GameState terminal_state = state;
      terminal_state.terminal = true;
      terminal_state.winner = -1;
      return TerminalUtility(terminal_state);
    }

    const int player = state.current_player;
    const auto owner = (player == 0) ? core::NodeOwner::kPlayer0 : core::NodeOwner::kPlayer1;
    const std::string key = core::InfoSetKey(state, owner);

    const auto legal = core::LegalActions(state).actions;
    if (legal.empty()) {
      throw std::logic_error("Non-terminal state with no legal actions");
    }

    auto& node = nodes_[key];
    if (node.actions.empty()) {
      node.actions = legal;
      node.regret_sum.assign(legal.size(), 0.0);
      node.strategy_sum.assign(legal.size(), 0.0);
    } else if (node.actions.size() != legal.size()) {
      throw std::logic_error("Action count mismatch for infoset: " + key);
    }

    const auto strategy = RegretMatching(node.regret_sum);
    const double reach = (player == 0) ? p0_reach : p1_reach;
    for (std::size_t i = 0; i < node.strategy_sum.size(); ++i) {
      node.strategy_sum[i] += reach * strategy[i];
    }

    std::vector<double> util(legal.size(), 0.0);
    double node_util = 0.0;
    for (std::size_t i = 0; i < legal.size(); ++i) {
      core::GameState next = state;
      core::ApplyAction(next, legal[i]);
      if (player == 0) {
        util[i] = Cfr(next, p0_reach * strategy[i], p1_reach);
      } else {
        util[i] = Cfr(next, p0_reach, p1_reach * strategy[i]);
      }
      node_util += strategy[i] * util[i];
    }

    const double opp_reach = (player == 0) ? p1_reach : p0_reach;
    for (std::size_t i = 0; i < legal.size(); ++i) {
      const double regret = (player == 0) ? (util[i] - node_util) : (node_util - util[i]);
      node.regret_sum[i] += opp_reach * regret;
    }

    return node_util;
  }

  NlheCfrOptions options_;
  std::unordered_map<std::string, CfrNode> nodes_;
};

}  // namespace poker_solver::solver

