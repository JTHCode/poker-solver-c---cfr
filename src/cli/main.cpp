#include <cstdlib>
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
#include "util/logging.h"
#include "util/rng.h"
#include "util/timer.h"

namespace {

struct CliOptions {
  std::optional<int> number_of_situations;
  std::string output = "out.jsonl";
  std::optional<std::uint64_t> seed;
  bool show_help = false;
  bool valid = true;
  std::string error_message;
};

void PrintHelp(std::string_view program_name) {
  std::cout << "Usage: " << program_name
            << " --number_of_situations <n> [--output <path>] [--seed <seed>] [--help]\n"
            << "\n"
            << "Options:\n"
            << "  -n, --number_of_situations <n>  Number of random situations to solve (required).\n"
            << "  -o, --output <path>             Output JSONL path (default: out.jsonl).\n"
            << "  -s, --seed <seed>               RNG seed (recommended for reproducibility).\n"
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
  poker_solver::util::Timer timer;

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
  const int max_attempts = target * 50;

  while (written < target) {
    if (attempts++ >= max_attempts) {
      LOG_ERROR("Exceeded max attempts while deduping; stopping early.");
      break;
    }

    auto scenario = poker_solver::solver::GenerateScenario(rng, scenario_config);
    const std::string spot_id = poker_solver::solver::SpotIdString(scenario);
    if (existing.find(spot_id) != existing.end()) {
      continue;
    }
    existing.insert(spot_id);

    poker_solver::io::SpotJsonInput out;
    out.id = written + 1;
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
    out.solve_time_ms = timer.ElapsedMillis();

    writer.AppendLine(poker_solver::io::BuildSpotJsonLine(out));
    ++written;
  }

  LOG_INFO("Wrote " + std::to_string(written) + " unique spot(s) to " + options.output + ".");

  return EXIT_SUCCESS;
}
