#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>

#include "core/cards.h"
#include "solver/scenario_generator.h"
#include "util/hash.h"

namespace poker_solver::solver {

inline std::string CanonicalTwoCards(const core::Card& a, const core::Card& b) {
  const auto ida = core::ToId(a);
  const auto idb = core::ToId(b);
  if (ida < idb) {
    return core::ToString(a) + core::ToString(b);
  }
  return core::ToString(b) + core::ToString(a);
}

inline std::string SpotIdString(const Scenario& scenario) {
  // Deterministic ID for dedupe across runs: depends only on public+private cards and positions.
  // Excludes RNG seed by design.
  std::string canonical;
  canonical.reserve(128);
  canonical += "opener=" + ToString(scenario.opener_position);
  canonical += "|defender=" + ToString(scenario.defender_position);
  canonical += "|hero_is_opener=" + std::string(scenario.hero_is_opener ? "1" : "0");
  canonical += "|preflop=" + scenario.preflop_action_line;

  const std::string hero_cards = CanonicalTwoCards(scenario.hero_cards[0], scenario.hero_cards[1]);
  const std::string villain_cards =
      CanonicalTwoCards(scenario.villain_cards[0], scenario.villain_cards[1]);
  canonical += "|hero=" + hero_cards;
  canonical += "|villain=" + villain_cards;

  canonical += "|board=";
  canonical += core::ToString(scenario.board.flop[0]);
  canonical += core::ToString(scenario.board.flop[1]);
  canonical += core::ToString(scenario.board.flop[2]);
  canonical += core::ToString(scenario.board.turn);
  canonical += core::ToString(scenario.board.river);

  const std::uint64_t h = util::Fnv1a64(canonical);
  return util::ToHex(h);
}

}  // namespace poker_solver::solver
