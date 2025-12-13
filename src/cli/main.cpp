#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "util/logging.h"

namespace {

struct CliOptions {
  std::optional<int> number_of_situations;
  bool show_help = false;
  bool valid = true;
  std::string error_message;
};

void PrintHelp(std::string_view program_name) {
  std::cout << "Usage: " << program_name << " --number_of_situations <n> [--help]\n"
            << "\n"
            << "Options:\n"
            << "  -n, --number_of_situations <n>  Number of random situations to solve (required).\n"
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

  LOG_INFO("Solver CLI scaffold initialized.");
  LOG_INFO("Requested situations: " + std::to_string(*options.number_of_situations));
  LOG_INFO("Solver logic not yet implemented (Phase 1 scaffold).");

  return EXIT_SUCCESS;
}
