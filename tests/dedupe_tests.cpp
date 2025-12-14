#include <cassert>
#include <filesystem>
#include <string>
#include <unordered_set>

#include "io/jsonl_dedupe.h"
#include "io/jsonl_writer.h"
#include "io/spot_json.h"
#include "solver/scenario_generator.h"
#include "solver/spot_id.h"
#include "util/rng.h"

namespace {

using poker_solver::io::BuildSpotJsonLine;
using poker_solver::io::JsonlWriter;
using poker_solver::io::LoadExistingSpotIds;
using poker_solver::io::SpotJsonInput;
using poker_solver::solver::GenerateScenario;
using poker_solver::solver::ScenarioConfig;
using poker_solver::solver::SpotIdString;
using poker_solver::util::Rng;

void TestSpotIdStableAndDedupeScan() {
  ScenarioConfig config;
  config.project_root = std::string(PROJECT_SOURCE_DIR);
  config.force_hero_is_opener = true;
  config.force_opener_position = poker_solver::solver::Position::kUTG;
  config.force_defender_position = poker_solver::solver::Position::kBB;
  config.force_preflop_line = poker_solver::solver::PreflopLine::kOpenCall;

  Rng rng1(999);
  const auto s1 = GenerateScenario(rng1, config);
  const auto id1 = SpotIdString(s1);

  Rng rng2(999);
  const auto s2 = GenerateScenario(rng2, config);
  const auto id2 = SpotIdString(s2);

  assert(id1 == id2);

  const std::filesystem::path out_path = "dedupe_tests_output.jsonl";
  std::error_code ec;
  std::filesystem::remove(out_path, ec);

  {
    JsonlWriter writer(out_path.string());
    SpotJsonInput line;
    line.id = 1;
    line.seed = 1;
    line.spot_id = id1;
    line.hero_pos = "UTG";
    line.villain_pos = "BB";
    line.preflop_action_line = "UTG_OPEN_CALL";
    line.hero_cards = "AsKd";
    line.villain_cards = "QcQd";
    line.flop0 = "2c";
    line.flop1 = "7d";
    line.flop2 = "Th";
    line.turn = "Jc";
    line.river = "9s";
    writer.AppendLine(BuildSpotJsonLine(line));
    writer.AppendLine(BuildSpotJsonLine(line));  // duplicate spot_id
  }

  const auto ids = LoadExistingSpotIds(out_path.string());
  assert(ids.size() == 1);
  assert(ids.find(id1) != ids.end());

  std::filesystem::remove(out_path, ec);
}

}  // namespace

int main() {
  TestSpotIdStableAndDedupeScan();
  return 0;
}

