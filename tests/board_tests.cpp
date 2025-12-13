#include <cassert>
#include <stdexcept>
#include <vector>

#include "core/board_gen.h"
#include "util/rng.h"

namespace {

using poker_solver::core::Board;
using poker_solver::core::Card;
using poker_solver::core::GenerateBoard;
using poker_solver::core::Rank;
using poker_solver::core::StandardDeck;
using poker_solver::core::Suit;
using poker_solver::core::ToId;
using poker_solver::core::ToString;
using poker_solver::util::Rng;

void TestStandardDeckUnique() {
  const auto deck = StandardDeck();
  assert(deck.size() == 52);
  std::vector<bool> seen(52, false);
  for (const auto& card : deck) {
    const auto id = ToId(card);
    assert(id < 52);
    assert(!seen[id]);
    seen[id] = true;
    assert(!ToString(card).empty());
  }
}

void AssertUniqueBoard(const Board& board) {
  std::vector<bool> seen(52, false);
  auto mark = [&](const Card& c) {
    const auto id = ToId(c);
    assert(!seen[id]);
    seen[id] = true;
  };
  mark(board.flop[0]);
  mark(board.flop[1]);
  mark(board.flop[2]);
  mark(board.turn);
  mark(board.river);
}

void TestBoardGenerationDeterministic() {
  Rng rng1(123);
  const auto board1 = GenerateBoard(rng1);

  Rng rng2(123);
  const auto board2 = GenerateBoard(rng2);

  AssertUniqueBoard(board1);
  AssertUniqueBoard(board2);
  assert(board1.flop[0] == board2.flop[0]);
  assert(board1.flop[1] == board2.flop[1]);
  assert(board1.flop[2] == board2.flop[2]);
  assert(board1.turn == board2.turn);
  assert(board1.river == board2.river);
}

void TestBoardGenerationWithBlockers() {
  Rng rng(999);
  std::vector<Card> blockers{
      Card{Rank::kAce, Suit::kSpades},
      Card{Rank::kKing, Suit::kHearts},
  };
  const auto board = GenerateBoard(rng, blockers);
  AssertUniqueBoard(board);

  const auto is_blocker = [&](const Card& c) {
    return (c == blockers[0]) || (c == blockers[1]);
  };

  assert(!is_blocker(board.flop[0]));
  assert(!is_blocker(board.flop[1]));
  assert(!is_blocker(board.flop[2]));
  assert(!is_blocker(board.turn));
  assert(!is_blocker(board.river));
}

void TestDuplicateBlockerThrows() {
  Rng rng(42);
  std::vector<Card> blockers{
      Card{Rank::kAce, Suit::kSpades},
      Card{Rank::kAce, Suit::kSpades},
  };
  bool threw = false;
  try {
    (void)GenerateBoard(rng, blockers);
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  assert(threw);
}

}  // namespace

int main() {
  TestStandardDeckUnique();
  TestBoardGenerationDeterministic();
  TestBoardGenerationWithBlockers();
  TestDuplicateBlockerThrows();
  return 0;
}
