#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include "core/cards.h"
#include "core/game_state.h"
#include "core/tree_builder.h"

namespace {

using poker_solver::core::Board;
using poker_solver::core::BuildTree;
using poker_solver::core::Card;
using poker_solver::core::GameState;
using poker_solver::core::NodeOwner;
using poker_solver::core::Rank;
using poker_solver::core::Street;
using poker_solver::core::Suit;
using poker_solver::core::ToId;
using poker_solver::core::TreeBuildOptions;

std::uint64_t HashTreeShape(const poker_solver::core::Tree& tree) {
  std::uint64_t h = 1469598103934665603ull;
  const auto mix = [&](std::uint64_t v) {
    h ^= v;
    h *= 1099511628211ull;
  };
  mix(static_cast<std::uint64_t>(tree.nodes.size()));
  mix(static_cast<std::uint64_t>(tree.root_id));
  for (const auto& n : tree.nodes) {
    mix(static_cast<std::uint64_t>(n.owner));
    mix(static_cast<std::uint64_t>(n.state.street));
    mix(static_cast<std::uint64_t>(n.state.board_count));
    mix(static_cast<std::uint64_t>(n.state.pot));
    mix(static_cast<std::uint64_t>(n.state.to_call));
    mix(static_cast<std::uint64_t>(n.children.size()));
    mix(static_cast<std::uint64_t>(n.info_set_key.size()));
  }
  return h;
}

void TestChanceNodesAdvanceStreetAndAvoidBlockers() {
  GameState root{};
  root.street = Street::kFlop;
  root.board.flop[0] = Card{Rank::kTwo, Suit::kClubs};
  root.board.flop[1] = Card{Rank::kThree, Suit::kDiamonds};
  root.board.flop[2] = Card{Rank::kFour, Suit::kHearts};
  root.board_count = 3;
  root.stacks[0] = 0;
  root.stacks[1] = 0;
  root.pot = 10;
  root.to_call = 0;
  root.current_player = 0;

  const std::vector<Card> blockers{
      Card{Rank::kAce, Suit::kHearts},   Card{Rank::kAce, Suit::kDiamonds},
      Card{Rank::kKing, Suit::kSpades},  Card{Rank::kKing, Suit::kClubs},
  };

  TreeBuildOptions opt;
  opt.chance_samples = 0;  // enumerate
  opt.max_nodes = 20000;

  const auto tree = BuildTree(root, blockers, opt);

  // Find the first chance node (end of flop after check-check).
  int chance_id = -1;
  for (const auto& n : tree.nodes) {
    if (n.owner == NodeOwner::kChance) {
      chance_id = n.id;
      break;
    }
  }
  assert(chance_id != -1);
  const auto& chance = tree.nodes[static_cast<std::size_t>(chance_id)];
  assert(chance.state.street == Street::kFlop);
  assert(chance.state.street_complete);
  assert(!chance.state.terminal);

  // Remaining deck size at flop: 52 - 4 hole - 3 flop = 45.
  assert(chance.children.size() == 45);

  std::vector<bool> seen(52, false);
  for (const auto& b : blockers) {
    seen[ToId(b)] = true;
  }
  seen[ToId(root.board.flop[0])] = true;
  seen[ToId(root.board.flop[1])] = true;
  seen[ToId(root.board.flop[2])] = true;

  std::vector<bool> seen_turn(52, false);
  for (const int child_id : chance.children) {
    const auto& child = tree.nodes[static_cast<std::size_t>(child_id)];
    assert(child.owner == NodeOwner::kPlayer0);
    assert(child.state.street == Street::kTurn);
    assert(child.state.board_count == 4);
    const auto turn_id = ToId(child.state.board.turn);
    assert(!seen[turn_id]);
    assert(!seen_turn[turn_id]);
    seen_turn[turn_id] = true;
    // New street must start clean.
    assert(child.state.to_call == 0);
    assert(child.state.street_committed[0] == 0);
    assert(child.state.street_committed[1] == 0);
    assert(child.state.last_bet_size == 0);
    assert(child.state.raises_this_street == 0);
    assert(child.state.reopen_allowed);
    assert(!child.state.street_complete);
    assert(!child.state.terminal);
  }
}

void TestTreeDeterministicUnderSampling() {
  GameState root{};
  root.street = Street::kFlop;
  root.board.flop[0] = Card{Rank::kTwo, Suit::kClubs};
  root.board.flop[1] = Card{Rank::kThree, Suit::kDiamonds};
  root.board.flop[2] = Card{Rank::kFour, Suit::kHearts};
  root.board_count = 3;
  root.stacks[0] = 0;
  root.stacks[1] = 0;
  root.pot = 10;
  root.to_call = 0;
  root.current_player = 0;

  const std::vector<Card> blockers{
      Card{Rank::kAce, Suit::kHearts},   Card{Rank::kAce, Suit::kDiamonds},
      Card{Rank::kKing, Suit::kSpades},  Card{Rank::kKing, Suit::kClubs},
  };

  TreeBuildOptions opt;
  opt.chance_seed = 12345;
  opt.chance_samples = 7;
  opt.max_nodes = 20000;

  const auto t1 = BuildTree(root, blockers, opt);
  const auto t2 = BuildTree(root, blockers, opt);

  assert(t1.nodes.size() == t2.nodes.size());
  assert(HashTreeShape(t1) == HashTreeShape(t2));
}

}  // namespace

int main() {
  TestChanceNodesAdvanceStreetAndAvoidBlockers();
  TestTreeDeterministicUnderSampling();
  return 0;
}

