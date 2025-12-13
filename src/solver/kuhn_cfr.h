#pragma once

#include <array>
#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace poker_solver::solver {

// Minimal CFR trainer for Kuhn poker (3-card deck J,Q,K; 1-chip ante; 1-chip bet).
// Returns utilities from player 0 perspective.
class KuhnCfrTrainer {
 public:
  // Strategy is [0]=check/call, [1]=bet/fold (context dependent).
  using Strategy = std::array<double, 2>;

  void Train(int iterations) {
    if (iterations <= 0) {
      throw std::invalid_argument("iterations must be positive");
    }
    for (int i = 0; i < iterations; ++i) {
      for (const auto& deal : Deals()) {
        const double chance_prob = 1.0 / 6.0;
        (void)Cfr(deal.first, deal.second, "", 1.0, 1.0, chance_prob);
      }
    }
  }

  Strategy AverageStrategy(std::string_view info_set_key) const {
    const auto it = info_sets_.find(std::string(info_set_key));
    if (it == info_sets_.end()) {
      return {0.5, 0.5};
    }
    const auto& node = it->second;
    const double sum = node.strategy_sum[0] + node.strategy_sum[1];
    if (sum <= 0.0) {
      return {0.5, 0.5};
    }
    return {node.strategy_sum[0] / sum, node.strategy_sum[1] / sum};
  }

 private:
  struct InfoSetNode {
    Strategy regret_sum{0.0, 0.0};
    Strategy strategy_sum{0.0, 0.0};
  };

  static constexpr std::array<std::pair<char, char>, 6> Deals() {
    // All distinct 2-card deals (player0, player1): 3*2 = 6.
    return {{{'J', 'Q'}, {'J', 'K'}, {'Q', 'J'}, {'Q', 'K'}, {'K', 'J'}, {'K', 'Q'}}};
  }

  static bool IsTerminal(std::string_view history) {
    return history == "cc" || history == "bc" || history == "bf" || history == "cbc" ||
           history == "cbf";
  }

  static bool Showdown(std::string_view history) {
    return history == "cc" || history == "bc" || history == "cbc";
  }

  static int CurrentPlayer(std::string_view history) {
    // Player 0 acts at start; players alternate each action.
    return static_cast<int>(history.size() % 2);
  }

  static Strategy LegalActionMask(std::string_view history) {
    // Actions are {pass/check/call, bet/fold} depending on state.
    // After a bet: [call, fold]. Otherwise: [check, bet].
    if (history.empty() || history == "c") {
      return {1.0, 1.0};  // check or bet
    }
    if (history == "b" || history == "cb") {
      return {1.0, 1.0};  // call or fold
    }
    throw std::invalid_argument("Unexpected history for legal actions: " + std::string(history));
  }

  static double TerminalUtility(char card_p0, char card_p1, std::string_view history) {
    const auto rank = [](char c) -> int {
      switch (c) {
        case 'J':
          return 0;
        case 'Q':
          return 1;
        case 'K':
          return 2;
        default:
          throw std::invalid_argument("Invalid Kuhn card");
      }
    };
    const bool p0_wins = rank(card_p0) > rank(card_p1);

    if (history == "bf") {
      return 1.0;  // p0 bet, p1 folded
    }
    if (history == "cbf") {
      return -1.0;  // p1 bet, p0 folded
    }
    if (Showdown(history)) {
      const double win_amount = (history == "cc") ? 1.0 : 2.0;
      return p0_wins ? win_amount : -win_amount;
    }
    throw std::invalid_argument("Non-terminal history passed to TerminalUtility");
  }

  static std::string InfoSetKey(char player_card, std::string_view history) {
    // Standard Kuhn infoset: private card + public action history.
    std::string key;
    key.reserve(2 + history.size());
    key.push_back(player_card);
    key.push_back(':');
    key.append(history.data(), history.size());
    return key;
  }

  static Strategy RegretMatching(const Strategy& regrets, const Strategy& mask) {
    Strategy positive{0.0, 0.0};
    for (int a = 0; a < 2; ++a) {
      const double r = regrets[static_cast<std::size_t>(a)];
      if (r > 0.0 && mask[static_cast<std::size_t>(a)] > 0.0) {
        positive[static_cast<std::size_t>(a)] = r;
      }
    }
    const double sum = positive[0] + positive[1];
    if (sum > 0.0) {
      return {positive[0] / sum, positive[1] / sum};
    }

    // Uniform over legal actions.
    const double legal = mask[0] + mask[1];
    if (legal <= 0.0) {
      return {0.5, 0.5};
    }
    return {mask[0] / legal, mask[1] / legal};
  }

  double Cfr(char card_p0, char card_p1, std::string_view history, double p0_reach, double p1_reach,
             double chance_prob) {
    if (IsTerminal(history)) {
      return TerminalUtility(card_p0, card_p1, history);
    }

    const int player = CurrentPlayer(history);
    const char card = (player == 0) ? card_p0 : card_p1;
    const std::string info_key = InfoSetKey(card, history);
    auto& node = info_sets_[info_key];

    const Strategy legal_mask = LegalActionMask(history);
    const Strategy strategy = RegretMatching(node.regret_sum, legal_mask);

    // Track average strategy for current player.
    const double reach = (player == 0) ? p0_reach : p1_reach;
    node.strategy_sum[0] += chance_prob * reach * strategy[0];
    node.strategy_sum[1] += chance_prob * reach * strategy[1];

    std::array<double, 2> util{0.0, 0.0};
    double node_util = 0.0;
    for (int a = 0; a < 2; ++a) {
      if (legal_mask[static_cast<std::size_t>(a)] <= 0.0) {
        continue;
      }
      const char action_char = ActionChar(history, a);
      const std::string next_history = std::string(history) + action_char;

      if (player == 0) {
        util[static_cast<std::size_t>(a)] =
            Cfr(card_p0, card_p1, next_history, p0_reach * strategy[static_cast<std::size_t>(a)],
                p1_reach, chance_prob);
      } else {
        util[static_cast<std::size_t>(a)] =
            Cfr(card_p0, card_p1, next_history, p0_reach,
                p1_reach * strategy[static_cast<std::size_t>(a)], chance_prob);
      }
      node_util += strategy[static_cast<std::size_t>(a)] * util[static_cast<std::size_t>(a)];
    }

    // Regret updates scaled by opponent reach.
    const double opp_reach = (player == 0) ? p1_reach : p0_reach;
    for (int a = 0; a < 2; ++a) {
      if (legal_mask[static_cast<std::size_t>(a)] <= 0.0) {
        continue;
      }
      const double regret = util[static_cast<std::size_t>(a)] - node_util;
      if (player == 0) {
        node.regret_sum[static_cast<std::size_t>(a)] += chance_prob * opp_reach * regret;
      } else {
        // Player 1 utility is -util, so regret flips sign.
        node.regret_sum[static_cast<std::size_t>(a)] += chance_prob * opp_reach * (-regret);
      }
    }

    return node_util;
  }

  static char ActionChar(std::string_view history, int action_index) {
    // action_index 0 => check/call ('c'); action_index 1 => bet/fold ('b' or 'f')
    if (action_index == 0) {
      return 'c';
    }
    if (history == "b" || history == "cb") {
      return 'f';
    }
    return 'b';
  }

  std::unordered_map<std::string, InfoSetNode> info_sets_;
};

}  // namespace poker_solver::solver
