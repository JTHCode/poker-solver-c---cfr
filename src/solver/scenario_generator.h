#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <cmath>
#include <cctype>

#include "core/board_gen.h"
#include "core/cards.h"
#include "core/game_state.h"
#include "io/preflop_range.h"
#include "util/rng.h"

namespace poker_solver::solver {

enum class Position { kUTG, kHJ, kCO, kBTN, kSB, kBB };

inline std::string ToString(Position position) {
  switch (position) {
    case Position::kUTG:
      return "UTG";
    case Position::kHJ:
      return "HJ";
    case Position::kCO:
      return "CO";
    case Position::kBTN:
      return "BTN";
    case Position::kSB:
      return "SB";
    case Position::kBB:
      return "BB";
  }
  throw std::invalid_argument("Unknown position");
}

inline std::string ToLower(Position position) {
  const auto upper = ToString(position);
  std::string lower;
  lower.reserve(upper.size());
  for (char c : upper) {
    lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  }
  return lower;
}

enum class PreflopLine { kOpenCall, kOpen3BetCall };

struct ScenarioConfig {
  std::string project_root;  // if set, range paths are relative to this directory
  int stack_bb{100};
  int units_per_bb{2};  // default: 1 unit = 0.5bb (supports 2.5bb open exactly)
  double open_bb{2.5};
  double three_bet_bb{10.0};
  std::optional<PreflopLine> force_preflop_line;

  std::optional<Position> force_opener_position;
  std::optional<Position> force_defender_position;
  std::optional<bool> force_hero_is_opener;
};

struct Scenario {
  Position opener_position{Position::kUTG};
  Position defender_position{Position::kBB};
  bool hero_is_opener{true};
  PreflopLine preflop_line{PreflopLine::kOpenCall};
  std::string preflop_action_line;

  io::PreflopRange opener_range{};
  io::PreflopRange defender_range{};

  core::Card hero_cards[2]{};
  core::Card villain_cards[2]{};
  core::Board board{};

  core::GameState root_state{};
};

namespace detail {

inline int ToUnits(double bb_amount, int units_per_bb) {
  return static_cast<int>(std::lround(bb_amount * static_cast<double>(units_per_bb)));
}

inline std::string PreflopLineToString(PreflopLine line) {
  switch (line) {
    case PreflopLine::kOpenCall:
      return "OPEN_CALL";
    case PreflopLine::kOpen3BetCall:
      return "OPEN_3BET_CALL";
  }
  throw std::invalid_argument("Unknown preflop line");
}

inline std::string JoinPath(std::string_view root, std::string_view relative) {
  if (root.empty()) {
    return std::string(relative);
  }
  std::string out(root);
  if (out.back() != '/') {
    out.push_back('/');
  }
  out.append(relative.data(), relative.size());
  return out;
}

inline std::string OpenRangePath(std::string_view root, Position opener_position) {
  const std::string rel = "preflop-ranges/100bb/open/" + ToString(opener_position) + "_OPEN.json";
  return JoinPath(root, rel);
}

inline std::string VsOpenRangePath(std::string_view root, Position defender_position,
                                   Position opener_position) {
  const std::string rel = "preflop-ranges/100bb/vs_open/vs_" + ToLower(opener_position) + "/" +
                          ToString(defender_position) + "_" + ToString(opener_position) +
                          "_VS_OPEN.json";
  return JoinPath(root, rel);
}

inline std::vector<Position> AvailableDefenders(Position opener_position) {
  switch (opener_position) {
    case Position::kUTG:
      return {Position::kHJ, Position::kCO, Position::kBTN, Position::kSB, Position::kBB};
    case Position::kHJ:
      return {Position::kCO, Position::kBTN, Position::kSB, Position::kBB};
    case Position::kCO:
      return {Position::kBTN, Position::kSB, Position::kBB};
    case Position::kBTN:
      return {Position::kSB, Position::kBB};
    case Position::kSB:
      return {Position::kBB};
    case Position::kBB:
      return {};
  }
  return {};
}

inline Position SampleOpenerPosition(util::Rng& rng) {
  // Open ranges exist for these positions.
  static constexpr Position kOpeners[] = {Position::kUTG, Position::kHJ, Position::kCO,
                                         Position::kBTN, Position::kSB};
  return kOpeners[rng.UniformInt(0, static_cast<int>(std::size(kOpeners) - 1))];
}

inline Position SampleDefenderPosition(util::Rng& rng, Position opener) {
  const auto defenders = AvailableDefenders(opener);
  if (defenders.empty()) {
    throw std::runtime_error("No defenders available for opener position");
  }
  const int idx = rng.UniformInt(0, static_cast<int>(defenders.size() - 1));
  return defenders[static_cast<std::size_t>(idx)];
}

inline std::vector<core::Card> CollectBlockers(const Scenario& scenario) {
  return {scenario.hero_cards[0], scenario.hero_cards[1], scenario.villain_cards[0],
          scenario.villain_cards[1]};
}

}  // namespace detail

inline Scenario GenerateScenario(util::Rng& rng, const ScenarioConfig& config = {}) {
  Scenario scenario;
  scenario.preflop_line =
      config.force_preflop_line.value_or(rng.UniformInt(0, 1) == 0 ? PreflopLine::kOpenCall
                                                                   : PreflopLine::kOpen3BetCall);

  scenario.opener_position =
      config.force_opener_position.value_or(detail::SampleOpenerPosition(rng));
  scenario.defender_position = config.force_defender_position.value_or(
      detail::SampleDefenderPosition(rng, scenario.opener_position));

  if (scenario.opener_position == scenario.defender_position) {
    throw std::logic_error("Opener and defender positions must differ");
  }

  scenario.hero_is_opener =
      config.force_hero_is_opener.value_or(rng.UniformInt(0, 1) == 0 ? false : true);

  scenario.opener_range =
      io::LoadPreflopRange(detail::OpenRangePath(config.project_root, scenario.opener_position));
  scenario.defender_range = io::LoadPreflopRange(
      detail::VsOpenRangePath(config.project_root, scenario.defender_position, scenario.opener_position));

  const int stack_units = config.stack_bb * config.units_per_bb;
  const int open_units = detail::ToUnits(config.open_bb, config.units_per_bb);
  const int three_bet_units = detail::ToUnits(config.three_bet_bb, config.units_per_bb);

  const int committed_units =
      (scenario.preflop_line == PreflopLine::kOpenCall) ? open_units : three_bet_units;

  if (committed_units <= 0 || committed_units >= stack_units) {
    throw std::invalid_argument("Invalid committed amount for stacks");
  }

  scenario.preflop_action_line =
      ToString(scenario.opener_position) + "_" + detail::PreflopLineToString(scenario.preflop_line);

  // Sample hole cards from the two distributions, avoiding collisions.
  const auto opener_hand = io::SampleHand(scenario.opener_range, rng);
  std::vector<core::Card> blockers{opener_hand.first, opener_hand.second};
  const auto defender_hand = io::SampleHand(scenario.defender_range, rng, blockers);

  if (scenario.hero_is_opener) {
    scenario.hero_cards[0] = opener_hand.first;
    scenario.hero_cards[1] = opener_hand.second;
    scenario.villain_cards[0] = defender_hand.first;
    scenario.villain_cards[1] = defender_hand.second;
  } else {
    scenario.hero_cards[0] = defender_hand.first;
    scenario.hero_cards[1] = defender_hand.second;
    scenario.villain_cards[0] = opener_hand.first;
    scenario.villain_cards[1] = opener_hand.second;
  }

  // Generate a random board, blocked by both players' hole cards.
  scenario.board = core::GenerateBoard(rng, detail::CollectBlockers(scenario));

  // Build an initial postflop game state.
  core::GameState state;
  state.street = core::Street::kFlop;
  state.pot = 2 * committed_units;
  state.to_call = 0;
  state.stacks[0] = stack_units - committed_units;
  state.stacks[1] = stack_units - committed_units;
  state.committed[0] = committed_units;
  state.committed[1] = committed_units;
  state.last_bet_size = 0;
  state.current_player = 0;
  state.terminal = false;
  state.street_complete = false;
  state.consecutive_checks = 0;
  state.winner = -1;
  scenario.root_state = state;

  return scenario;
}

}  // namespace poker_solver::solver
