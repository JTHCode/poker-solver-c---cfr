#pragma once

#include <algorithm>
#include <stdexcept>
#include <vector>

#include "core/board.h"
#include "core/cards.h"
#include "util/rng.h"

namespace poker_solver::core {

inline Board GenerateBoard(util::Rng& rng, const std::vector<Card>& blockers = {}) {
  auto deck = StandardDeck();

  // Remove blockers from deck, error on duplicates.
  std::vector<bool> seen(52, false);
  for (const auto& blocker : blockers) {
    const auto id = ToId(blocker);
    if (seen[id]) {
      throw std::invalid_argument("Duplicate blocker card provided.");
    }
    seen[id] = true;
    deck.erase(std::remove_if(deck.begin(), deck.end(),
                              [id](const Card& c) { return ToId(c) == id; }),
               deck.end());
  }

  if (deck.size() < 5) {
    throw std::invalid_argument("Not enough cards remaining to generate a board.");
  }

  rng.Shuffle(deck);

  Board board;
  board.flop[0] = deck[0];
  board.flop[1] = deck[1];
  board.flop[2] = deck[2];
  board.turn = deck[3];
  board.river = deck[4];
  return board;
}

}  // namespace poker_solver::core
