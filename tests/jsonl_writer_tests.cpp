#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "io/jsonl_writer.h"
#include "io/minijson.h"
#include "io/spot_json.h"

namespace {

using poker_solver::io::BuildSpotJsonLine;
using poker_solver::io::JsonlWriter;
using poker_solver::io::SpotJsonInput;
using poker_solver::io::minijson::Parse;
using poker_solver::io::minijson::RequireObjectKey;

std::vector<std::string> ReadAllLines(const std::filesystem::path& path) {
  std::ifstream in(path);
  assert(in.good());
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(in, line)) {
    lines.push_back(line);
  }
  return lines;
}

void TestJsonlWriterWritesValidJsonObjects() {
  const std::filesystem::path out_path =
      std::filesystem::path("jsonl_writer_tests_output.jsonl");
  std::error_code ec;
  std::filesystem::remove(out_path, ec);

  {
    JsonlWriter writer(out_path.string());

    SpotJsonInput a;
    a.id = 1;
    a.seed = 42;
    a.spot_id = "deadbeefdeadbeef";
    a.hero_pos = "BTN";
    a.villain_pos = "BB";
    a.preflop_action_line = "BTN_OPEN_CALL";
    a.hero_cards = "AsKd";
    a.villain_cards = "QcQd";
    a.flop0 = "2c";
    a.flop1 = "7d";
    a.flop2 = "Th";
    a.turn = "Jc";
    a.river = "9s";
    a.solve_time_ms = 12.5;
    a.root_action_labels = {"CHECK:0", "BET:1"};
    a.root_action_probs = {0.4, 0.6};
    SpotJsonInput::SolvedBranch br;
    br.hero_root_action_label = "BET:1";
    br.root_probability = 0.6;
    br.hero_root_strategy_after_lock = {0.0, 1.0};
    a.solved_branches.push_back(br);

    SpotJsonInput b = a;
    b.id = 2;
    b.seed = 43;
    b.spot_id = "feedfacefeedface";
    b.hero_cards = "AhAd";
    b.villain_cards = "KsKd";

    writer.AppendLine(BuildSpotJsonLine(a));
    writer.AppendLine(BuildSpotJsonLine(b));
    writer.Flush();
  }

  const auto lines = ReadAllLines(out_path);
  assert(lines.size() == 2);
  for (const auto& line : lines) {
    const auto json = Parse(line);
    assert(json.IsObject());
    const auto& root = json.AsObject();
    const auto& metadata = RequireObjectKey(root, "metadata");
    const auto& ranges = RequireObjectKey(root, "ranges");
    const auto& board = RequireObjectKey(root, "board");
    const auto& metrics = RequireObjectKey(root, "metrics");

    assert(metadata.IsObject());
    assert(ranges.IsObject());
    const auto& root_strategy = RequireObjectKey(root, "root_strategy");
    const auto& solved_branches = RequireObjectKey(root, "solved_branches");
    assert(board.IsObject());
    assert(metrics.IsObject());
    assert(root_strategy.IsObject());
    assert(solved_branches.IsArray());

    const auto& meta_obj = metadata.AsObject();
    assert(RequireObjectKey(meta_obj, "id").IsNumber());
    assert(RequireObjectKey(meta_obj, "seed").IsNumber());
    assert(RequireObjectKey(meta_obj, "hero_pos").IsString());
    assert(RequireObjectKey(meta_obj, "villain_pos").IsString());
    assert(RequireObjectKey(meta_obj, "preflop_action_line").IsString());
    assert(RequireObjectKey(meta_obj, "spot_id").IsString());
  }

  std::filesystem::remove(out_path, ec);
}

}  // namespace

int main() {
  TestJsonlWriterWritesValidJsonObjects();
  return 0;
}
