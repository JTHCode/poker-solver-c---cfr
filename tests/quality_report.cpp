#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "core/cards.h"
#include "core/game_state.h"
#include "solver/kuhn_cfr.h"
#include "solver/kuhn_metrics.h"
#include "solver/nlhe_cfr.h"
#include "solver/nlhe_metrics.h"

namespace {

using poker_solver::core::Card;
using poker_solver::core::GameState;
using poker_solver::core::Rank;
using poker_solver::core::Street;
using poker_solver::core::Suit;
using poker_solver::solver::HoldemTerminalContext;
using poker_solver::solver::KuhnCfrTrainer;
using poker_solver::solver::KuhnExploitability;
using poker_solver::solver::NlheCfrOptions;
using poker_solver::solver::NlheCfrSolver;
using poker_solver::solver::NlheExploitabilityRiverOnly;

struct Options {
  int kuhn_iterations = 20000;
  int nlhe_iterations = 400;
  std::uint64_t seed = 12345;
};

void PrintHelp(const char* argv0) {
  std::cout << "Usage: " << argv0 << " [--kuhn_iterations N] [--nlhe_iterations N] [--seed N]\n";
}

Options ParseArgs(int argc, char** argv) {
  Options opt;
  for (int i = 1; i < argc; ++i) {
    const std::string a(argv[i]);
    if (a == "--help" || a == "-h") {
      PrintHelp(argv[0]);
      std::exit(EXIT_SUCCESS);
    }
    if (a == "--kuhn_iterations" && i + 1 < argc) {
      opt.kuhn_iterations = std::stoi(argv[++i]);
      continue;
    }
    if (a == "--nlhe_iterations" && i + 1 < argc) {
      opt.nlhe_iterations = std::stoi(argv[++i]);
      continue;
    }
    if (a == "--seed" && i + 1 < argc) {
      opt.seed = static_cast<std::uint64_t>(std::stoull(argv[++i]));
      continue;
    }
    throw std::invalid_argument("Unknown/invalid arg: " + a);
  }
  if (opt.kuhn_iterations <= 0 || opt.nlhe_iterations <= 0) {
    throw std::invalid_argument("iterations must be positive");
  }
  return opt;
}

GameState MakeRiverRootState() {
  GameState s{};
  s.street = Street::kRiver;

  s.board.flop[0] = Card{Rank::kTwo, Suit::kClubs};
  s.board.flop[1] = Card{Rank::kThree, Suit::kDiamonds};
  s.board.flop[2] = Card{Rank::kFour, Suit::kHearts};
  s.board.turn = Card{Rank::kFive, Suit::kSpades};
  s.board.river = Card{Rank::kSeven, Suit::kClubs};
  s.board_count = 5;

  s.pot = 20;
  s.committed[0] = 10;
  s.committed[1] = 10;
  s.stacks[0] = 100;
  s.stacks[1] = 100;

  s.to_call = 0;
  s.street_committed[0] = 0;
  s.street_committed[1] = 0;
  s.last_bet_size = 0;
  s.raises_this_street = 0;
  s.reopen_allowed = true;
  s.consecutive_checks = 0;
  s.current_player = 0;
  s.terminal = false;
  s.street_complete = false;
  s.winner = -1;
  return s;
}

HoldemTerminalContext MakeHoldemContext(const GameState& s, std::uint64_t seed) {
  HoldemTerminalContext ctx;
  ctx.p0_hole = {Card{Rank::kAce, Suit::kSpades}, Card{Rank::kAce, Suit::kHearts}};
  ctx.p1_hole = {Card{Rank::kKing, Suit::kClubs}, Card{Rank::kKing, Suit::kDiamonds}};
  ctx.board = s.board;
  ctx.board_count = 5;
  ctx.board_samples = 0;
  ctx.runout_seed = seed;
  poker_solver::solver::PrepareHoldemContext(ctx);
  return ctx;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const auto opt = ParseArgs(argc, argv);

    KuhnCfrTrainer kuhn;
    kuhn.Train(opt.kuhn_iterations);
    const auto k = KuhnExploitability(kuhn);

    const auto river = MakeRiverRootState();
    NlheCfrOptions nopt;
    nopt.iterations = opt.nlhe_iterations;
    nopt.showdown_winner = 0;
    nopt.holdem = MakeHoldemContext(river, opt.seed);
    NlheCfrSolver nlhe(nopt);
    nlhe.Solve(river);
    const auto n = NlheExploitabilityRiverOnly(river, nlhe, nopt);

    std::cout << "Kuhn exploitability: " << k.exploitability << " (v=" << k.value_p0 << ", br0=" << k.br0_value_p0
              << ", br1=" << k.br1_value_p0 << ")\n";
    std::cout << "NLHE river-only exploitability: " << n.exploitability << " (v=" << n.value_p0
              << ", br0=" << n.br0_value_p0 << ", br1=" << n.br1_value_p0 << ")\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    PrintHelp(argv[0]);
    return 2;
  }
}
