#pragma once

#include <algorithm>
#include <array>
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
  int node_reserve{0};                           // if >0, reserve node table capacity
};

struct CfrNode {
  std::vector<core::Action> actions;
  std::vector<double> regret_sum;
  std::vector<double> strategy_sum;
};

class NlheCfrSolver {
 public:
  struct InfoKey {
    std::uint8_t owner{0};  // 0=P0, 1=P1
    std::uint8_t street{0};
    std::uint8_t board_count{0};
    std::uint8_t current_player{0};
    std::uint8_t consecutive_checks{0};
    std::uint8_t reopen_allowed{0};
    std::uint8_t terminal{0};
    std::uint8_t street_complete{0};
    int pot{0};
    int to_call{0};
    int last_bet_size{0};
    int street_committed0{0};
    int street_committed1{0};
    int raises_this_street{0};
    std::array<std::uint8_t, 5> board_ids{{255, 255, 255, 255, 255}};

    bool operator==(const InfoKey& other) const {
      return owner == other.owner && street == other.street && board_count == other.board_count &&
             current_player == other.current_player && consecutive_checks == other.consecutive_checks &&
             reopen_allowed == other.reopen_allowed && terminal == other.terminal &&
             street_complete == other.street_complete && pot == other.pot && to_call == other.to_call &&
             last_bet_size == other.last_bet_size && street_committed0 == other.street_committed0 &&
             street_committed1 == other.street_committed1 && raises_this_street == other.raises_this_street &&
             board_ids == other.board_ids;
    }
  };

  struct InfoKeyHasher {
    std::size_t operator()(const InfoKey& k) const noexcept {
      std::size_t h = 0;
      const auto mix = [&](std::size_t v) {
        h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
      };
      mix(static_cast<std::size_t>(k.owner));
      mix(static_cast<std::size_t>(k.street));
      mix(static_cast<std::size_t>(k.board_count));
      mix(static_cast<std::size_t>(k.current_player));
      mix(static_cast<std::size_t>(k.consecutive_checks));
      mix(static_cast<std::size_t>(k.reopen_allowed));
      mix(static_cast<std::size_t>(k.terminal));
      mix(static_cast<std::size_t>(k.street_complete));
      mix(static_cast<std::size_t>(static_cast<std::uint32_t>(k.pot)));
      mix(static_cast<std::size_t>(static_cast<std::uint32_t>(k.to_call)));
      mix(static_cast<std::size_t>(static_cast<std::uint32_t>(k.last_bet_size)));
      mix(static_cast<std::size_t>(static_cast<std::uint32_t>(k.street_committed0)));
      mix(static_cast<std::size_t>(static_cast<std::uint32_t>(k.street_committed1)));
      mix(static_cast<std::size_t>(static_cast<std::uint32_t>(k.raises_this_street)));
      for (const auto id : k.board_ids) {
        mix(static_cast<std::size_t>(id));
      }
      return h;
    }
  };

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
    root_key_ = MakeKey(root_state, core::NodeOwner::kPlayer0);
    stats_ = Stats{};
    runout_cache_.clear();
    nodes_.clear();
    if (options_.node_reserve > 0) {
      nodes_.reserve(static_cast<std::size_t>(options_.node_reserve));
    }
    for (int i = 0; i < options_.iterations; ++i) {
      (void)Cfr(root_state, 1.0, 1.0);
    }
  }

  std::vector<double> AverageStrategy(const core::GameState& state, core::NodeOwner owner) const {
    const auto it = nodes_.find(MakeKey(state, owner));
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

  std::vector<core::Action> ActionsForInfoSet(const core::GameState& state, core::NodeOwner owner) const {
    const auto it = nodes_.find(MakeKey(state, owner));
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
    const InfoKey key = MakeKey(state, owner);
    auto [it, inserted] = nodes_.try_emplace(key);
    auto& node = it->second;
    if (inserted) {
      node.actions = core::LegalActions(state).actions;
      if (node.actions.empty()) {
        throw std::logic_error("Non-terminal state with no legal actions");
      }
      node.regret_sum.assign(node.actions.size(), 0.0);
      node.strategy_sum.assign(node.actions.size(), 0.0);
    }

    const auto& legal = node.actions;
    stats_.legal_actions_total += static_cast<std::uint64_t>(legal.size());

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

  static InfoKey MakeKey(const core::GameState& state, core::NodeOwner owner) {
    InfoKey k;
    k.owner = static_cast<std::uint8_t>(owner == core::NodeOwner::kPlayer1 ? 1 : 0);
    k.street = static_cast<std::uint8_t>(state.street);
    k.board_count = static_cast<std::uint8_t>(state.board_count);
    k.current_player = static_cast<std::uint8_t>(state.current_player);
    k.consecutive_checks = static_cast<std::uint8_t>(state.consecutive_checks);
    k.reopen_allowed = static_cast<std::uint8_t>(state.reopen_allowed ? 1 : 0);
    k.terminal = static_cast<std::uint8_t>(state.terminal ? 1 : 0);
    k.street_complete = static_cast<std::uint8_t>(state.street_complete ? 1 : 0);
    k.pot = state.pot;
    k.to_call = state.to_call;
    k.last_bet_size = state.last_bet_size;
    k.street_committed0 = state.street_committed[0];
    k.street_committed1 = state.street_committed[1];
    k.raises_this_street = state.raises_this_street;

    if (state.board_count >= 1) k.board_ids[0] = core::ToId(state.board.flop[0]);
    if (state.board_count >= 2) k.board_ids[1] = core::ToId(state.board.flop[1]);
    if (state.board_count >= 3) k.board_ids[2] = core::ToId(state.board.flop[2]);
    if (state.board_count >= 4) k.board_ids[3] = core::ToId(state.board.turn);
    if (state.board_count >= 5) k.board_ids[4] = core::ToId(state.board.river);
    return k;
  }

  NlheCfrOptions options_;
  std::unordered_map<InfoKey, CfrNode, InfoKeyHasher> nodes_;
  core::GameState root_state_{};
  InfoKey root_key_{};
  mutable Stats stats_{};
  mutable std::unordered_map<std::uint64_t, double> runout_cache_{};
};

}  // namespace poker_solver::solver
