#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/board.h"
#include "core/cards.h"
#include "core/hand_evaluator.h"
#include "core/hand_rank.h"
#include "util/rng.h"
#include "util/hash.h"

namespace poker_solver::solver {

struct HoldemTerminalContext {
  std::array<core::Card, 2> p0_hole{};
  std::array<core::Card, 2> p1_hole{};
  core::Board board{};
  int board_count{5};           // number of known board cards in order flop(3),turn,river
  int board_samples{0};         // if >0, sample runouts when board_count < 5
  std::uint64_t runout_seed{0}; // seed used for sampling runouts
  int full_board_result{-2};    // when board_count==5: -2 unknown, -1 tie, 0 p0 wins, 1 p1 wins
};

struct RunoutStats {
  std::uint64_t chance_samples{0};  // number of complete-board evaluations performed
};

inline double FoldUtilityP0(int winner, int committed0, int committed1) {
  if (winner == 0) {
    return static_cast<double>(committed1);
  }
  if (winner == 1) {
    return -static_cast<double>(committed0);
  }
  throw std::invalid_argument("winner must be 0 or 1 for fold utility");
}

inline double ShowdownUtilityP0(const std::array<core::Card, 2>& p0_hole,
                                const std::array<core::Card, 2>& p1_hole, const core::Board& board,
                                int committed0, int committed1) {
  const auto r0 = core::EvaluateHoldem7(p0_hole, board);
  const auto r1 = core::EvaluateHoldem7(p1_hole, board);
  if (r0 > r1) {
    return FoldUtilityP0(0, committed0, committed1);
  }
  if (r1 > r0) {
    return FoldUtilityP0(1, committed0, committed1);
  }
  // Split pot: net utility for player0 is half-pot minus own committed.
  return (static_cast<double>(committed1) - static_cast<double>(committed0)) / 2.0;
}

inline double ShowdownUtilityP0FromResult(int result, int committed0, int committed1) {
  if (result == 0 || result == 1) {
    return FoldUtilityP0(result, committed0, committed1);
  }
  if (result == -1) {
    return (static_cast<double>(committed1) - static_cast<double>(committed0)) / 2.0;
  }
  throw std::invalid_argument("Invalid full_board_result");
}

inline void PrepareHoldemContext(HoldemTerminalContext& ctx) {
  if (ctx.board_count != 5) {
    return;
  }
  if (ctx.full_board_result != -2) {
    return;
  }
  const auto r0 = core::EvaluateHoldem7(ctx.p0_hole, ctx.board);
  const auto r1 = core::EvaluateHoldem7(ctx.p1_hole, ctx.board);
  if (r0 > r1) {
    ctx.full_board_result = 0;
  } else if (r1 > r0) {
    ctx.full_board_result = 1;
  } else {
    ctx.full_board_result = -1;
  }
}

inline std::vector<core::Card> KnownBoardCards(const core::Board& board, int board_count) {
  if (board_count < 0 || board_count > 5) {
    throw std::invalid_argument("board_count must be in [0,5]");
  }
  std::vector<core::Card> out;
  out.reserve(static_cast<std::size_t>(board_count));
  if (board_count >= 1) out.push_back(board.flop[0]);
  if (board_count >= 2) out.push_back(board.flop[1]);
  if (board_count >= 3) out.push_back(board.flop[2]);
  if (board_count >= 4) out.push_back(board.turn);
  if (board_count >= 5) out.push_back(board.river);
  return out;
}

inline std::uint64_t RunoutCacheKey(const HoldemTerminalContext& ctx, int committed0, int committed1) {
  // Hash all fields that affect the expected showdown utility.
  std::string s;
  s.reserve(128);
  s += util::ToHex(static_cast<std::uint64_t>(ctx.board_count));
  s += "|";
  s += core::ToString(ctx.p0_hole[0]) + core::ToString(ctx.p0_hole[1]);
  s += "|";
  s += core::ToString(ctx.p1_hole[0]) + core::ToString(ctx.p1_hole[1]);
  s += "|";
  for (const auto& c : KnownBoardCards(ctx.board, ctx.board_count)) {
    s += core::ToString(c);
  }
  s += "|";
  s += std::to_string(committed0) + ":" + std::to_string(committed1);
  s += "|";
  s += std::to_string(ctx.board_samples);
  s += "|";
  s += util::ToHex(ctx.runout_seed);
  return util::Fnv1a64(s);
}

inline core::Board BuildBoardFromKnownAndRunout(const std::vector<core::Card>& known,
                                                const std::vector<core::Card>& runout) {
  if (known.size() + runout.size() != 5) {
    throw std::invalid_argument("known+runout must be 5 cards");
  }
  std::vector<core::Card> all = known;
  all.insert(all.end(), runout.begin(), runout.end());
  core::Board board;
  board.flop[0] = all[0];
  board.flop[1] = all[1];
  board.flop[2] = all[2];
  board.turn = all[3];
  board.river = all[4];
  return board;
}

inline std::vector<core::Card> RemainingDeck(const HoldemTerminalContext& ctx) {
  auto deck = core::StandardDeck();
  const auto erase_card = [&](const core::Card& c) {
    const auto id = core::ToId(c);
    deck.erase(std::remove_if(deck.begin(), deck.end(),
                              [id](const core::Card& d) { return core::ToId(d) == id; }),
               deck.end());
  };
  erase_card(ctx.p0_hole[0]);
  erase_card(ctx.p0_hole[1]);
  erase_card(ctx.p1_hole[0]);
  erase_card(ctx.p1_hole[1]);
  for (const auto& c : KnownBoardCards(ctx.board, ctx.board_count)) {
    erase_card(c);
  }
  return deck;
}

inline double ExpectedShowdownUtilityP0(const HoldemTerminalContext& ctx, int committed0, int committed1,
                                       std::unordered_map<std::uint64_t, double>& cache,
                                       RunoutStats& stats) {
  if (ctx.board_count == 5) {
    if (ctx.full_board_result != -2) {
      return ShowdownUtilityP0FromResult(ctx.full_board_result, committed0, committed1);
    }
    stats.chance_samples += 1;
    return ShowdownUtilityP0(ctx.p0_hole, ctx.p1_hole, ctx.board, committed0, committed1);
  }

  const int remaining = 5 - ctx.board_count;
  if (remaining <= 0) {
    throw std::invalid_argument("Invalid board_count");
  }

  const std::uint64_t key = RunoutCacheKey(ctx, committed0, committed1);
  const auto it = cache.find(key);
  if (it != cache.end()) {
    return it->second;
  }

  const auto known = KnownBoardCards(ctx.board, ctx.board_count);
  auto deck = RemainingDeck(ctx);

  double sum = 0.0;
  std::uint64_t samples = 0;

  const auto eval_runout = [&](const std::vector<core::Card>& runout_cards) {
    const auto full_board = BuildBoardFromKnownAndRunout(known, runout_cards);
    sum += ShowdownUtilityP0(ctx.p0_hole, ctx.p1_hole, full_board, committed0, committed1);
    ++samples;
  };

  if (remaining <= 2 && ctx.board_samples == 0) {
    // Exact enumeration for 1 or 2 unknown cards.
    if (remaining == 1) {
      for (std::size_t i = 0; i < deck.size(); ++i) {
        eval_runout({deck[i]});
      }
    } else {
      for (std::size_t i = 0; i < deck.size(); ++i) {
        for (std::size_t j = i + 1; j < deck.size(); ++j) {
          eval_runout({deck[i], deck[j]});
        }
      }
    }
  } else {
    if (ctx.board_samples <= 0) {
      throw std::invalid_argument("board_samples must be > 0 when board_count < 5 and remaining > 2");
    }
    util::Rng rng(ctx.runout_seed);
    std::vector<core::Card> runout_cards;
    runout_cards.resize(static_cast<std::size_t>(remaining));
    for (int s = 0; s < ctx.board_samples; ++s) {
      rng.Shuffle(deck);
      for (int i = 0; i < remaining; ++i) {
        runout_cards[static_cast<std::size_t>(i)] = deck[static_cast<std::size_t>(i)];
      }
      eval_runout(runout_cards);
    }
  }

  if (samples == 0) {
    throw std::logic_error("No runout samples computed");
  }

  stats.chance_samples += samples;
  const double ev = sum / static_cast<double>(samples);
  cache.emplace(key, ev);
  return ev;
}

}  // namespace poker_solver::solver
