#pragma once

#include <sstream>
#include <string>
#include <vector>

#include "core/game_state.h"

namespace poker_solver::core {

enum class NodeOwner { kChance, kPlayer0, kPlayer1, kTerminal };

struct Node {
  int id{0};
  NodeOwner owner{NodeOwner::kPlayer0};
  GameState state{};
  bool terminal{false};
  std::vector<int> children;
  std::string info_set_key;
};

inline std::string InfoSetKey(const GameState& state, NodeOwner owner) {
  std::ostringstream oss;
  oss << static_cast<int>(state.street) << "|";
  oss << (owner == NodeOwner::kPlayer0 ? "P0" : owner == NodeOwner::kPlayer1 ? "P1"
                                                                             : "C")  // chance
      << "|";
  oss << state.pot << "|" << state.to_call << "|" << state.last_bet_size << "|" << state.current_player
      << "|" << state.consecutive_checks << "|" << (state.street_complete ? 1 : 0) << "|"
      << (state.terminal ? 1 : 0);
  return oss.str();
}

}  // namespace poker_solver::core
