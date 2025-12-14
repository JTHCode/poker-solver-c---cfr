#include <cassert>
#include <vector>

#include "core/board.h"
#include "core/cards.h"
#include "solver/scenario_generator.h"
#include "util/rng.h"

namespace {

using poker_solver::core::Card;
using poker_solver::core::ToId;
using poker_solver::solver::GenerateScenario;
using poker_solver::solver::Position;
using poker_solver::solver::PreflopLine;
using poker_solver::solver::ScenarioConfig;
using poker_solver::util::Rng;

void AssertNoCollisions(const std::vector<Card>& cards) {
  std::vector<bool> seen(52, false);
  for (const auto& c : cards) {
    const auto id = ToId(c);
    assert(id < 52);
    assert(!seen[id]);
    seen[id] = true;
  }
}

void TestScenarioDeterministicAndValid() {
  ScenarioConfig config;
  config.project_root = std::string(PROJECT_SOURCE_DIR);
  config.force_preflop_line = PreflopLine::kOpenCall;
  config.force_opener_position = Position::kUTG;
  config.force_defender_position = Position::kBB;
  config.force_hero_is_opener = true;

  Rng rng1(12345);
  const auto s1 = GenerateScenario(rng1, config);

  Rng rng2(12345);
  const auto s2 = GenerateScenario(rng2, config);

  assert(s1.hero_cards[0] == s2.hero_cards[0]);
  assert(s1.hero_cards[1] == s2.hero_cards[1]);
  assert(s1.villain_cards[0] == s2.villain_cards[0]);
  assert(s1.villain_cards[1] == s2.villain_cards[1]);
  assert(s1.board.flop[0] == s2.board.flop[0]);
  assert(s1.board.flop[1] == s2.board.flop[1]);
  assert(s1.board.flop[2] == s2.board.flop[2]);
  assert(s1.board.turn == s2.board.turn);
  assert(s1.board.river == s2.board.river);

  std::vector<Card> all_cards{{
      s1.hero_cards[0],
      s1.hero_cards[1],
      s1.villain_cards[0],
      s1.villain_cards[1],
      s1.board.flop[0],
      s1.board.flop[1],
      s1.board.flop[2],
      s1.board.turn,
      s1.board.river,
  }};
  AssertNoCollisions(all_cards);

  assert(s1.root_state.street == poker_solver::core::Street::kFlop);
  assert(s1.root_state.to_call == 0);
  assert(s1.root_state.pot == s1.root_state.committed[0] + s1.root_state.committed[1]);
  assert(s1.root_state.stacks[0] >= 0);
  assert(s1.root_state.stacks[1] >= 0);
}

void TestScenarioThreeBetValid() {
  ScenarioConfig config;
  config.project_root = std::string(PROJECT_SOURCE_DIR);
  config.force_preflop_line = PreflopLine::kOpen3BetCall;
  config.force_opener_position = Position::kBTN;
  config.force_defender_position = Position::kBB;
  config.force_hero_is_opener = false;

  Rng rng(7);
  const auto s = GenerateScenario(rng, config);
  assert(s.root_state.pot == s.root_state.committed[0] + s.root_state.committed[1]);
  assert(s.root_state.committed[0] > 0);
}

}  // namespace

int main() {
  TestScenarioDeterministicAndValid();
  TestScenarioThreeBetValid();
  return 0;
}
