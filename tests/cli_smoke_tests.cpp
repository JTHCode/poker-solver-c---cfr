#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_set>

#include "io/minijson.h"

namespace {

constexpr const char* kSolverPath = SOLVER_PATH;

std::string QuoteShellArg(const std::string& arg) { return "\"" + arg + "\""; }

void AssertHasObjectKey(const poker_solver::io::minijson::Object& obj, const std::string& key) {
  const auto it = obj.find(key);
  assert(it != obj.end());
  assert(it->second.IsObject());
}

void AssertRootStrategyValid(const poker_solver::io::minijson::Object& obj) {
  const auto it = obj.find("root_strategy");
  assert(it != obj.end());
  assert(it->second.IsObject());
  const auto& rs = it->second.AsObject();

  const auto it_actions = rs.find("actions");
  const auto it_probs = rs.find("probs");
  assert(it_actions != rs.end());
  assert(it_probs != rs.end());
  assert(it_actions->second.IsArray());
  assert(it_probs->second.IsArray());

  const auto& actions = it_actions->second.AsArray();
  const auto& probs = it_probs->second.AsArray();
  assert(!actions.empty());
  assert(actions.size() == probs.size());

  double sum = 0.0;
  for (const auto& p : probs) {
    assert(p.IsNumber());
    const double v = p.AsNumber();
    assert(v >= -1e-12);
    sum += v;
  }
  assert(std::abs(sum - 1.0) < 1e-6);
}

void AssertSolvedBranchesValid(const poker_solver::io::minijson::Object& obj) {
  const auto it_rs = obj.find("root_strategy");
  assert(it_rs != obj.end());
  const auto& rs = it_rs->second.AsObject();
  const auto& actions = rs.at("actions").AsArray();

  const auto it = obj.find("solved_branches");
  assert(it != obj.end());
  assert(it->second.IsArray());
  const auto& branches = it->second.AsArray();
  for (const auto& b : branches) {
    assert(b.IsObject());
    const auto& br = b.AsObject();
    assert(br.at("hero_root_action").IsString());
    assert(br.at("root_probability").IsNumber());
    assert(br.at("hero_root_strategy_after_lock").IsArray());
    const auto& locked = br.at("hero_root_strategy_after_lock").AsArray();
    assert(locked.size() == actions.size());
  }
}

}  // namespace

int main() {
  assert(kSolverPath != nullptr);
  assert(std::string(kSolverPath).size() > 0);

  const std::filesystem::path out_path =
      std::filesystem::temp_directory_path() / "poker_solver_cli_smoke.jsonl";
  std::error_code ec;
  std::filesystem::remove(out_path, ec);

  const std::string cmd =
      QuoteShellArg(kSolverPath) + " --number_of_situations 3 --iterations 50 --branch_threshold 0.20" +
      " --seed 12345 --progress_every 1000000000 --output " + QuoteShellArg(out_path.string());
  const int rc = std::system(cmd.c_str());
  assert(rc == 0);

  std::ifstream in(out_path);
  assert(in.good());

  int lines = 0;
  std::unordered_set<std::string> seen_spot_ids;
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) {
      continue;
    }
    ++lines;
    const auto v = poker_solver::io::minijson::Parse(line);
    assert(v.IsObject());
    const auto& obj = v.AsObject();

    AssertHasObjectKey(obj, "metadata");
    AssertHasObjectKey(obj, "ranges");
    AssertHasObjectKey(obj, "board");
    AssertHasObjectKey(obj, "metrics");
    AssertRootStrategyValid(obj);
    AssertSolvedBranchesValid(obj);

    const auto& meta = obj.at("metadata").AsObject();
    assert(meta.at("spot_id").IsString());
    const std::string spot_id = meta.at("spot_id").AsString();
    assert(!spot_id.empty());
    assert(seen_spot_ids.insert(spot_id).second);
  }

  assert(lines == 3);

  std::filesystem::remove(out_path, ec);
  return 0;
}

