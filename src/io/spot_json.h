#pragma once

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "io/json_utils.h"

namespace poker_solver::io {

inline std::string ActionLabel(std::string_view type, int amount) {
  return std::string(type) + ":" + std::to_string(amount);
}

struct SpotJsonInput {
  int id{0};
  std::uint64_t seed{0};
  std::string spot_id;
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

  std::vector<std::string> root_action_labels;
  std::vector<double> root_action_probs;

  struct SolvedBranch {
    std::string hero_root_action_label;
    double root_probability{0.0};
    std::vector<double> hero_root_strategy_after_lock;
  };
  std::vector<SolvedBranch> solved_branches;

  double solve_time_ms{0.0};
  int skipped_duplicates{0};
  int attempts{0};
};

inline std::string BuildSpotJsonLine(const SpotJsonInput& in) {
  using io::json::Quote;

  if (in.root_action_labels.size() != in.root_action_probs.size()) {
    throw std::invalid_argument("root_action_labels/probs size mismatch");
  }
  for (const auto& branch : in.solved_branches) {
    if (branch.hero_root_strategy_after_lock.size() != in.root_action_labels.size()) {
      throw std::invalid_argument("branch strategy size mismatch");
    }
  }

  std::ostringstream oss;
  oss << "{";
  oss << "\"metadata\":{";
  oss << "\"id\":" << in.id << ",";
  oss << "\"seed\":" << in.seed << ",";
  oss << "\"spot_id\":" << Quote(in.spot_id) << ",";
  oss << "\"hero_pos\":" << Quote(in.hero_pos) << ",";
  oss << "\"villain_pos\":" << Quote(in.villain_pos) << ",";
  oss << "\"preflop_action_line\":" << Quote(in.preflop_action_line);
  oss << "},";

  oss << "\"ranges\":{";
  oss << "\"hero_cards\":" << Quote(in.hero_cards) << ",";
  oss << "\"villain_cards\":" << Quote(in.villain_cards);
  oss << "},";

  oss << "\"root_strategy\":{";
  oss << "\"actions\":[";
  for (std::size_t i = 0; i < in.root_action_labels.size(); ++i) {
    if (i > 0) {
      oss << ",";
    }
    oss << Quote(in.root_action_labels[i]);
  }
  oss << "],";
  oss << "\"probs\":[";
  oss << std::setprecision(17);
  for (std::size_t i = 0; i < in.root_action_probs.size(); ++i) {
    if (i > 0) {
      oss << ",";
    }
    oss << in.root_action_probs[i];
  }
  oss << "]";
  oss << "},";

  oss << "\"solved_branches\":[";
  for (std::size_t b = 0; b < in.solved_branches.size(); ++b) {
    if (b > 0) {
      oss << ",";
    }
    const auto& br = in.solved_branches[b];
    oss << "{";
    oss << "\"hero_root_action\":" << Quote(br.hero_root_action_label) << ",";
    oss << "\"root_probability\":" << br.root_probability << ",";
    oss << "\"hero_root_strategy_after_lock\":[";
    for (std::size_t i = 0; i < br.hero_root_strategy_after_lock.size(); ++i) {
      if (i > 0) {
        oss << ",";
      }
      oss << br.hero_root_strategy_after_lock[i];
    }
    oss << "]";
    oss << "}";
  }
  oss << "],";

  oss << "\"board\":{";
  oss << "\"flop\":[" << Quote(in.flop0) << "," << Quote(in.flop1) << "," << Quote(in.flop2)
      << "],";
  oss << "\"turn\":" << Quote(in.turn) << ",";
  oss << "\"river\":" << Quote(in.river);
  oss << "},";

  oss << "\"metrics\":{";
  oss << "\"solve_time_ms\":" << in.solve_time_ms;
  oss << ",";
  oss << "\"skipped_duplicates\":" << in.skipped_duplicates << ",";
  oss << "\"attempts\":" << in.attempts;
  oss << "}";

  oss << "}";
  return oss.str();
}

}  // namespace poker_solver::io
