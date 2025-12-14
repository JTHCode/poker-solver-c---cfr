#include <cstdlib>
#include <csignal>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <cstdint>

#include "io/jsonl_dedupe.h"
#include "io/jsonl_writer.h"
#include "io/spot_json.h"
#include "solver/scenario_generator.h"
#include "solver/spot_id.h"
#include "solver/subtree_expansion.h"
#include "util/logging.h"
#include "util/rng.h"
#include "util/timer.h"

namespace {

volatile std::sig_atomic_t g_stop_requested = 0;

void HandleSignal(int /*signal*/) { g_stop_requested = 1; }

struct CliOptions {
  std::optional<int> number_of_situations;
  std::string output = "out.jsonl";
  std::optional<std::uint64_t> seed;
  int iterations = 2000;
  double branch_threshold = 0.20;
  int progress_every = 10;
  bool show_help = false;
  bool valid = true;
  std::string error_message;
};

void PrintHelp(std::string_view program_name) {
  std::cout << "Usage: " << program_name
            << " --number_of_situations <n> [--output <path>] [--seed <seed>] [--iterations <n>]\n"
            << "       [--branch_threshold <t>] [--progress_every <n>] [--help]\n"
            << "\n"
            << "Options:\n"
            << "  -n, --number_of_situations <n>  Number of random situations to solve (required).\n"
            << "  -o, --output <path>             Output JSONL path (default: out.jsonl).\n"
            << "  -s, --seed <seed>               RNG seed (recommended for reproducibility).\n"
            << "      --iterations <n>            CFR iterations per solve (default: 2000).\n"
            << "      --branch_threshold <t>      Root action threshold (default: 0.20).\n"
            << "      --progress_every <n>        Log progress every n written spots (default: 10).\n"
            << "  -h, --help                      Show this help message.\n";
}

CliOptions ParseArgs(int argc, char* argv[]) {
  CliOptions options;
  for (int i = 1; i < argc; ++i) {
    const std::string arg(argv[i]);
    if (arg == "-h" || arg == "--help") {
      options.show_help = true;
      continue;
    }
    if (arg == "-o" || arg == "--output") {
      if (i + 1 >= argc) {
        options.valid = false;
        options.error_message = "Missing value for output.";
        return options;
      }
      options.output = argv[++i];
      continue;
    }
    if (arg == "-s" || arg == "--seed") {
      if (i + 1 >= argc) {
        options.valid = false;
        options.error_message = "Missing value for seed.";
        return options;
      }
      try {
        options.seed = static_cast<std::uint64_t>(std::stoull(argv[++i]));
      } catch (const std::exception&) {
        options.valid = false;
        options.error_message = "Invalid integer for seed.";
        return options;
      }
      continue;
    }
    if (arg == "--iterations") {
      if (i + 1 >= argc) {
        options.valid = false;
        options.error_message = "Missing value for iterations.";
        return options;
      }
      try {
        options.iterations = std::stoi(argv[++i]);
        if (options.iterations <= 0) {
          options.valid = false;
          options.error_message = "iterations must be positive.";
          return options;
        }
      } catch (const std::exception&) {
        options.valid = false;
        options.error_message = "Invalid integer for iterations.";
        return options;
      }
      continue;
    }
    if (arg == "--branch_threshold") {
      if (i + 1 >= argc) {
        options.valid = false;
        options.error_message = "Missing value for branch_threshold.";
        return options;
      }
      try {
        options.branch_threshold = std::stod(argv[++i]);
        if (options.branch_threshold < 0.0 || options.branch_threshold > 1.0) {
          options.valid = false;
          options.error_message = "branch_threshold must be in [0,1].";
          return options;
        }
      } catch (const std::exception&) {
        options.valid = false;
        options.error_message = "Invalid number for branch_threshold.";
        return options;
      }
      continue;
    }
    if (arg == "--progress_every") {
      if (i + 1 >= argc) {
        options.valid = false;
        options.error_message = "Missing value for progress_every.";
        return options;
      }
      try {
        options.progress_every = std::stoi(argv[++i]);
        if (options.progress_every <= 0) {
          options.valid = false;
          options.error_message = "progress_every must be positive.";
          return options;
        }
      } catch (const std::exception&) {
        options.valid = false;
        options.error_message = "Invalid integer for progress_every.";
        return options;
      }
      continue;
    }
    if (arg == "-n" || arg == "--number_of_situations") {
      if (i + 1 >= argc) {
        options.valid = false;
        options.error_message = "Missing value for number_of_situations.";
        return options;
      }
      try {
        const int value = std::stoi(argv[++i]);
        if (value <= 0) {
          options.valid = false;
          options.error_message = "number_of_situations must be positive.";
          return options;
        }
        options.number_of_situations = value;
      } catch (const std::invalid_argument&) {
        options.valid = false;
        options.error_message = "Invalid integer for number_of_situations.";
        return options;
      } catch (const std::out_of_range&) {
        options.valid = false;
        options.error_message = "number_of_situations is out of range.";
        return options;
      }
      continue;
    }

    options.valid = false;
    options.error_message = "Unknown argument: " + arg;
    return options;
  }

  return options;
}

}  // namespace

int main(int argc, char* argv[]) {
  std::signal(SIGINT, HandleSignal);
  std::signal(SIGTERM, HandleSignal);

  const auto options = ParseArgs(argc, argv);

  if (!options.valid) {
    LOG_ERROR(options.error_message);
    PrintHelp(argv[0]);
    return EXIT_FAILURE;
  }

  if (options.show_help || !options.number_of_situations.has_value()) {
    PrintHelp(argv[0]);
    return options.show_help ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  const std::uint64_t seed = options.seed.value_or(42);
  poker_solver::util::Rng rng(seed);
  poker_solver::util::Timer overall_timer;

  poker_solver::solver::ScenarioConfig scenario_config;
  scenario_config.project_root = PROJECT_SOURCE_DIR;

  poker_solver::io::JsonlWriter writer(options.output);
  auto existing = poker_solver::io::LoadExistingSpotIds(options.output);
  if (!existing.empty()) {
    LOG_INFO("Loaded " + std::to_string(existing.size()) + " existing spot_id(s) from output.");
  }

  const int target = *options.number_of_situations;
  int written = 0;
  int attempts = 0;
  int skipped_duplicates = 0;
  const int max_attempts = target * 50;
  const int base_id = static_cast<int>(existing.size());

  while (written < target) {
    if (g_stop_requested != 0) {
      LOG_WARN("Stop requested; exiting after writing completed spots.");
      break;
    }
    if (attempts++ >= max_attempts) {
      LOG_ERROR("Exceeded max attempts while deduping; stopping early.");
      break;
    }

    poker_solver::util::Timer solve_timer;
    auto scenario = poker_solver::solver::GenerateScenario(rng, scenario_config);
    const std::string spot_id = poker_solver::solver::SpotIdString(scenario);
    if (existing.find(spot_id) != existing.end()) {
      ++skipped_duplicates;
      continue;
    }
    existing.insert(spot_id);

    const int spot_seq_id = base_id + written + 1;

    poker_solver::solver::NlheCfrOptions root_opt;
    root_opt.iterations = options.iterations;
    root_opt.showdown_winner = 0;  // fallback if no holdem context is provided
    poker_solver::solver::HoldemTerminalContext holdem;
    holdem.p0_hole = {scenario.hero_cards[0], scenario.hero_cards[1]};
    holdem.p1_hole = {scenario.villain_cards[0], scenario.villain_cards[1]};
    holdem.board = scenario.board;
    holdem.board_count = 5;
    holdem.board_samples = 0;
    holdem.runout_seed = seed ^ static_cast<std::uint64_t>(spot_seq_id);
    root_opt.holdem = holdem;

    const auto root_strategy = poker_solver::solver::SolveRootStrategy(scenario.root_state, root_opt);

    poker_solver::solver::NlheCfrOptions branch_opt = root_opt;
    branch_opt.iterations = options.iterations;
    const auto branches = poker_solver::solver::SolveBranches(
        scenario.root_state, root_strategy, options.branch_threshold, branch_opt);

    poker_solver::io::SpotJsonInput out;
    out.id = spot_seq_id;
    out.seed = seed;
    out.spot_id = spot_id;
    out.preflop_action_line = scenario.preflop_action_line;
    out.hero_pos = scenario.hero_is_opener ? poker_solver::solver::ToString(scenario.opener_position)
                                          : poker_solver::solver::ToString(scenario.defender_position);
    out.villain_pos =
        scenario.hero_is_opener ? poker_solver::solver::ToString(scenario.defender_position)
                                : poker_solver::solver::ToString(scenario.opener_position);
    out.hero_cards = poker_solver::solver::CanonicalTwoCards(scenario.hero_cards[0], scenario.hero_cards[1]);
    out.villain_cards =
        poker_solver::solver::CanonicalTwoCards(scenario.villain_cards[0], scenario.villain_cards[1]);
    out.flop0 = poker_solver::core::ToString(scenario.board.flop[0]);
    out.flop1 = poker_solver::core::ToString(scenario.board.flop[1]);
    out.flop2 = poker_solver::core::ToString(scenario.board.flop[2]);
    out.turn = poker_solver::core::ToString(scenario.board.turn);
    out.river = poker_solver::core::ToString(scenario.board.river);
    out.solve_time_ms = solve_timer.ElapsedMillis();
    out.skipped_duplicates = skipped_duplicates;
    out.attempts = attempts;
    out.branch_solves_executed = static_cast<int>(branches.size());
    out.chance_samples = static_cast<int>(root_strategy.stats.chance_samples);

    std::uint64_t nodes_visited = root_strategy.stats.nodes_visited;
    std::uint64_t decision_nodes = root_strategy.stats.decision_nodes;
    std::uint64_t terminal_evals = root_strategy.stats.terminal_evals;
    std::uint64_t legal_actions_total = root_strategy.stats.legal_actions_total;
    for (const auto& br : branches) {
      nodes_visited += br.stats.nodes_visited;
      decision_nodes += br.stats.decision_nodes;
      terminal_evals += br.stats.terminal_evals;
      legal_actions_total += br.stats.legal_actions_total;
      out.chance_samples += static_cast<int>(br.stats.chance_samples);
    }
    out.nodes_visited = nodes_visited;
    out.decision_nodes = decision_nodes;
    out.terminal_evals = terminal_evals;
    out.legal_actions_total = legal_actions_total;

    out.root_action_labels.reserve(root_strategy.actions.size());
    out.root_action_probs = root_strategy.probabilities;
    for (const auto& a : root_strategy.actions) {
      out.root_action_labels.push_back(poker_solver::io::ActionLabel(
          a.type == poker_solver::core::ActionType::kFold   ? "FOLD"
          : a.type == poker_solver::core::ActionType::kCheck ? "CHECK"
          : a.type == poker_solver::core::ActionType::kCall  ? "CALL"
          : a.type == poker_solver::core::ActionType::kBet   ? "BET"
          : a.type == poker_solver::core::ActionType::kRaise ? "RAISE"
                                                             : "ALLIN",
          a.amount));
    }

    for (const auto& br : branches) {
      poker_solver::io::SpotJsonInput::SolvedBranch sb;
      sb.root_probability = br.root_probability;
      sb.hero_root_strategy_after_lock = br.hero_root_avg_strategy_after_lock;
      sb.hero_root_action_label = poker_solver::io::ActionLabel(
          br.hero_root_action.type == poker_solver::core::ActionType::kFold   ? "FOLD"
          : br.hero_root_action.type == poker_solver::core::ActionType::kCheck ? "CHECK"
          : br.hero_root_action.type == poker_solver::core::ActionType::kCall  ? "CALL"
          : br.hero_root_action.type == poker_solver::core::ActionType::kBet   ? "BET"
          : br.hero_root_action.type == poker_solver::core::ActionType::kRaise ? "RAISE"
                                                                               : "ALLIN",
          br.hero_root_action.amount);
      out.solved_branches.push_back(std::move(sb));
    }

    writer.AppendLine(poker_solver::io::BuildSpotJsonLine(out));
    writer.Flush();  // ensure completed hands persist even if the process exits early
    ++written;

    if (options.progress_every > 0 && (written % options.progress_every == 0 || written == target)) {
      const double elapsed_s = overall_timer.ElapsedMillis() / 1000.0;
      LOG_INFO("Progress: wrote " + std::to_string(written) + "/" + std::to_string(target) +
               " (skipped_duplicates=" + std::to_string(skipped_duplicates) +
               ", attempts=" + std::to_string(attempts) + ", elapsed_s=" + std::to_string(elapsed_s) +
               ")");
    }
  }

  LOG_INFO("Wrote " + std::to_string(written) + " unique spot(s) to " + options.output + ".");

  return EXIT_SUCCESS;
}
