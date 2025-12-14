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
  oss << state.pot << "|" << state.to_call << "|" << state.last_bet_size << "|" << state.street_committed[0]
      << "|" << state.street_committed[1] << "|" << state.raises_this_street << "|"
      << (state.reopen_allowed ? 1 : 0) << "|" << state.current_player << "|" << state.consecutive_checks
      << "|" << (state.street_complete ? 1 : 0) << "|" << (state.terminal ? 1 : 0);
  oss << "|B" << state.board_count << "|";
  if (state.board_count >= 1) oss << ToString(state.board.flop[0]);
  if (state.board_count >= 2) oss << ToString(state.board.flop[1]);
  if (state.board_count >= 3) oss << ToString(state.board.flop[2]);
  if (state.board_count >= 4) oss << ToString(state.board.turn);
  if (state.board_count >= 5) oss << ToString(state.board.river);
  return oss.str();
}

}  // namespace poker_solver::core
