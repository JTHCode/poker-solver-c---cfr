#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>

#include "io/json_utils.h"

namespace poker_solver::io {

struct SpotJsonInput {
  int id{0};
  std::uint64_t seed{0};
  std::string hero_pos;
  std::string villain_pos;
  std::string preflop_action_line;

  std::string hero_cards;    // e.g. "AsKd"
  std::string villain_cards; // e.g. "QcQd"

  std::string flop0;
  std::string flop1;
  std::string flop2;
  std::string turn;
  std::string river;

  double solve_time_ms{0.0};
};

inline std::string BuildSpotJsonLine(const SpotJsonInput& in) {
  using io::json::Quote;

  std::ostringstream oss;
  oss << "{";
  oss << "\"metadata\":{";
  oss << "\"id\":" << in.id << ",";
  oss << "\"seed\":" << in.seed << ",";
  oss << "\"hero_pos\":" << Quote(in.hero_pos) << ",";
  oss << "\"villain_pos\":" << Quote(in.villain_pos) << ",";
  oss << "\"preflop_action_line\":" << Quote(in.preflop_action_line);
  oss << "},";

  oss << "\"ranges\":{";
  oss << "\"hero_cards\":" << Quote(in.hero_cards) << ",";
  oss << "\"villain_cards\":" << Quote(in.villain_cards);
  oss << "},";

  oss << "\"root_strategy\":{},";
  oss << "\"solved_branches\":[],";

  oss << "\"board\":{";
  oss << "\"flop\":[" << Quote(in.flop0) << "," << Quote(in.flop1) << "," << Quote(in.flop2)
      << "],";
  oss << "\"turn\":" << Quote(in.turn) << ",";
  oss << "\"river\":" << Quote(in.river);
  oss << "},";

  oss << "\"metrics\":{";
  oss << "\"solve_time_ms\":" << in.solve_time_ms;
  oss << "}";

  oss << "}";
  return oss.str();
}

}  // namespace poker_solver::io

