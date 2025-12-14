#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "solver/kuhn_cfr.h"

namespace poker_solver::solver {

// Exploitability for Kuhn is computed on the full game:
// - v(σ0,σ1): expected value for player0
// - BR0(σ1): max_{σ0'} v(σ0',σ1)
// - BR1(σ0): min_{σ1'} v(σ0,σ1')
// - exploitability := 0.5 * (BR0(σ1) - BR1(σ0))
struct KuhnExploitabilityResult {
  double value_p0{0.0};
  double br0_value_p0{0.0};
  double br1_value_p0{0.0};
  double exploitability{0.0};
};

namespace detail {

static constexpr std::array<std::pair<char, char>, 6> kDeals = {
    {{'J', 'Q'}, {'J', 'K'}, {'Q', 'J'}, {'Q', 'K'}, {'K', 'J'}, {'K', 'Q'}}};

inline bool IsTerminal(std::string_view history) {
  return history == "cc" || history == "bc" || history == "bf" || history == "cbc" || history == "cbf";
}

inline bool Showdown(std::string_view history) { return history == "cc" || history == "bc" || history == "cbc"; }

inline int CurrentPlayer(std::string_view history) { return static_cast<int>(history.size() % 2); }

inline double TerminalUtility(char card_p0, char card_p1, std::string_view history) {
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
    return 1.0;
  }
  if (history == "cbf") {
    return -1.0;
  }
  if (Showdown(history)) {
    const double win_amount = (history == "cc") ? 1.0 : 2.0;
    return p0_wins ? win_amount : -win_amount;
  }
  throw std::invalid_argument("Non-terminal history passed to TerminalUtility");
}

inline char ActionChar(std::string_view history, int action_index) {
  if (action_index == 0) {
    return 'c';
  }
  if (history == "b" || history == "cb") {
    return 'f';
  }
  return 'b';
}

inline std::string InfoSetKey(char player_card, std::string_view history) {
  std::string key;
  key.reserve(2 + history.size());
  key.push_back(player_card);
  key.push_back(':');
  key.append(history.data(), history.size());
  return key;
}

template <typename StrategyFn0, typename StrategyFn1>
inline double EvalForDeal(char card_p0, char card_p1, std::string_view history, const StrategyFn0& s0,
                          const StrategyFn1& s1) {
  if (IsTerminal(history)) {
    return TerminalUtility(card_p0, card_p1, history);
  }

  const int player = CurrentPlayer(history);
  const char card = (player == 0) ? card_p0 : card_p1;
  const auto strat = (player == 0) ? s0(card, history) : s1(card, history);

  double value = 0.0;
  for (int a = 0; a < 2; ++a) {
    const char action_char = ActionChar(history, a);
    const std::string next = std::string(history) + action_char;
    value += strat[static_cast<std::size_t>(a)] * EvalForDeal(card_p0, card_p1, next, s0, s1);
  }
  return value;
}

struct DeterministicPolicy {
  // Maps infosets to action index 0 or 1.
  // For player0: histories {"", "cb"}; for player1: {"c", "b"}.
  // Keys are in the same format as KuhnCfrTrainer: "<card>:<history>".
  std::unordered_map<std::string, int> action_by_infoset;
};

inline std::vector<std::string> Player0InfoSets() {
  std::vector<std::string> keys;
  keys.reserve(6);
  for (char c : {'J', 'Q', 'K'}) {
    keys.push_back(InfoSetKey(c, ""));
    keys.push_back(InfoSetKey(c, "cb"));
  }
  return keys;
}

inline std::vector<std::string> Player1InfoSets() {
  std::vector<std::string> keys;
  keys.reserve(6);
  for (char c : {'J', 'Q', 'K'}) {
    keys.push_back(InfoSetKey(c, "c"));
    keys.push_back(InfoSetKey(c, "b"));
  }
  return keys;
}

template <typename StrategyFn0, typename StrategyFn1>
inline double ExpectedValue(const StrategyFn0& s0, const StrategyFn1& s1) {
  constexpr double kChanceProb = 1.0 / 6.0;
  double value = 0.0;
  for (const auto& deal : kDeals) {
    value += kChanceProb * EvalForDeal(deal.first, deal.second, "", s0, s1);
  }
  return value;
}

inline DeterministicPolicy PolicyFromBits(const std::vector<std::string>& keys, std::uint32_t bits) {
  DeterministicPolicy p;
  p.action_by_infoset.reserve(keys.size());
  for (std::size_t i = 0; i < keys.size(); ++i) {
    const int a = (bits & (1u << static_cast<unsigned>(i))) ? 1 : 0;
    p.action_by_infoset.emplace(keys[i], a);
  }
  return p;
}

}  // namespace detail

inline KuhnExploitabilityResult KuhnExploitability(const KuhnCfrTrainer& trainer) {
  const auto mixed0 = [&](char card, std::string_view history) -> KuhnCfrTrainer::Strategy {
    return trainer.AverageStrategy(detail::InfoSetKey(card, history));
  };
  const auto mixed1 = mixed0;

  const double value = detail::ExpectedValue(mixed0, mixed1);

  // Best response for player0 against σ1: enumerate all deterministic policies for player0 (2^6).
  const auto p0_infosets = detail::Player0InfoSets();
  double br0 = -1e300;
  for (std::uint32_t bits = 0; bits < (1u << 6); ++bits) {
    const auto policy = detail::PolicyFromBits(p0_infosets, bits);
    const auto br0_strat = [&](char card, std::string_view history) -> KuhnCfrTrainer::Strategy {
      const auto it = policy.action_by_infoset.find(detail::InfoSetKey(card, history));
      if (it == policy.action_by_infoset.end()) {
        throw std::logic_error("Missing player0 infoset in policy");
      }
      return (it->second == 0) ? KuhnCfrTrainer::Strategy{1.0, 0.0} : KuhnCfrTrainer::Strategy{0.0, 1.0};
    };
    br0 = std::max(br0, detail::ExpectedValue(br0_strat, mixed1));
  }

  // Best response for player1 against σ0: enumerate deterministic policies for player1 (2^6).
  const auto p1_infosets = detail::Player1InfoSets();
  double br1 = 1e300;
  for (std::uint32_t bits = 0; bits < (1u << 6); ++bits) {
    const auto policy = detail::PolicyFromBits(p1_infosets, bits);
    const auto br1_strat = [&](char card, std::string_view history) -> KuhnCfrTrainer::Strategy {
      const auto it = policy.action_by_infoset.find(detail::InfoSetKey(card, history));
      if (it == policy.action_by_infoset.end()) {
        throw std::logic_error("Missing player1 infoset in policy");
      }
      return (it->second == 0) ? KuhnCfrTrainer::Strategy{1.0, 0.0} : KuhnCfrTrainer::Strategy{0.0, 1.0};
    };
    br1 = std::min(br1, detail::ExpectedValue(mixed0, br1_strat));
  }

  KuhnExploitabilityResult out;
  out.value_p0 = value;
  out.br0_value_p0 = br0;
  out.br1_value_p0 = br1;
  out.exploitability = 0.5 * (br0 - br1);
  if (!std::isfinite(out.exploitability)) {
    throw std::logic_error("Non-finite Kuhn exploitability computed");
  }
  return out;
}

}  // namespace poker_solver::solver
