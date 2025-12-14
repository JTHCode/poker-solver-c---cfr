#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "core/cards.h"
#include "core/game_state.h"
#include "core/tree.h"
#include "util/rng.h"

namespace poker_solver::core {

struct TreeBuildOptions {
  std::uint64_t chance_seed{0};
  int chance_samples{0};  // if >0, sample this many outcomes per chance node (without replacement)
  int max_nodes{200000};
};

struct Tree {
  std::vector<Node> nodes;
  int root_id{0};
};

namespace detail {

inline std::vector<Card> KnownBoardCards(const GameState& state) {
  std::vector<Card> out;
  out.reserve(static_cast<std::size_t>(state.board_count));
  if (state.board_count >= 1) out.push_back(state.board.flop[0]);
  if (state.board_count >= 2) out.push_back(state.board.flop[1]);
  if (state.board_count >= 3) out.push_back(state.board.flop[2]);
  if (state.board_count >= 4) out.push_back(state.board.turn);
  if (state.board_count >= 5) out.push_back(state.board.river);
  return out;
}

inline std::vector<Card> RemainingDeckForChance(const GameState& state, const std::vector<Card>& blockers) {
  auto deck = StandardDeck();
  std::vector<bool> seen(52, false);
  const auto erase_card = [&](const Card& c) {
    const auto id = ToId(c);
    if (seen[id]) {
      throw std::invalid_argument("Duplicate blocker/board card in chance context");
    }
    seen[id] = true;
    deck.erase(std::remove_if(deck.begin(), deck.end(),
                              [id](const Card& d) { return ToId(d) == id; }),
               deck.end());
  };
  for (const auto& c : blockers) {
    erase_card(c);
  }
  for (const auto& c : KnownBoardCards(state)) {
    erase_card(c);
  }
  return deck;
}

inline GameState AdvanceStreet(const GameState& state, const Card& dealt) {
  if (!state.street_complete || state.terminal) {
    throw std::logic_error("AdvanceStreet requires a completed, non-terminal street");
  }
  if (state.street != Street::kFlop && state.street != Street::kTurn) {
    throw std::invalid_argument("AdvanceStreet only supports flop->turn and turn->river");
  }

  GameState next = state;
  next.street_complete = false;
  next.consecutive_checks = 0;
  next.to_call = 0;
  next.last_bet_size = 0;
  next.raises_this_street = 0;
  next.reopen_allowed = true;
  next.street_committed[0] = 0;
  next.street_committed[1] = 0;
  next.current_player = 0;
  next.winner = -1;

  if (state.street == Street::kFlop) {
    next.street = Street::kTurn;
    next.board.turn = dealt;
    next.board_count = 4;
  } else {
    next.street = Street::kRiver;
    next.board.river = dealt;
    next.board_count = 5;
  }
  return next;
}

}  // namespace detail

inline Tree BuildTree(const GameState& root_state, const std::vector<Card>& blockers,
                      const TreeBuildOptions& options = {}) {
  if (options.max_nodes <= 0) {
    throw std::invalid_argument("max_nodes must be positive");
  }
  if (options.chance_samples < 0) {
    throw std::invalid_argument("chance_samples must be >= 0");
  }

  Tree tree;
  tree.nodes.reserve(1024);

  auto add_node = [&](NodeOwner owner, const GameState& state) -> int {
    if (static_cast<int>(tree.nodes.size()) >= options.max_nodes) {
      throw std::runtime_error("Tree exceeded max_nodes");
    }
    Node node;
    node.id = static_cast<int>(tree.nodes.size());
    node.owner = owner;
    node.state = state;
    node.terminal = state.terminal;
    if (owner == NodeOwner::kPlayer0 || owner == NodeOwner::kPlayer1) {
      node.info_set_key = InfoSetKey(state, owner);
    }
    tree.nodes.push_back(std::move(node));
    return static_cast<int>(tree.nodes.size() - 1);
  };

  std::function<int(const GameState&)> build = [&](const GameState& state) -> int {
    if (state.terminal) {
      return add_node(NodeOwner::kTerminal, state);
    }
    if (state.street_complete) {
      return add_node(NodeOwner::kChance, state);
    }
    const auto owner = (state.current_player == 0) ? NodeOwner::kPlayer0 : NodeOwner::kPlayer1;
    return add_node(owner, state);
  };

  tree.root_id = build(root_state);

  // Expand nodes in creation order; children are appended deterministically.
  for (std::size_t idx = 0; idx < tree.nodes.size(); ++idx) {
    if (tree.nodes[idx].terminal) {
      continue;
    }

    const NodeOwner owner = tree.nodes[idx].owner;
    const GameState state = tree.nodes[idx].state;
    std::vector<int> children;

    if (owner == NodeOwner::kChance) {
      if (!state.street_complete || state.terminal) {
        throw std::logic_error("Chance node has invalid state");
      }
      if (state.street == Street::kTurn || state.street == Street::kRiver) {
        // Street completion is handled by ApplyAction as terminal on river; but remain defensive.
        if (state.street == Street::kRiver) {
          GameState terminal = state;
          terminal.terminal = true;
          const int child = build(terminal);
          children.push_back(child);
          tree.nodes[idx].children = std::move(children);
          continue;
        }
      }
      if (state.street != Street::kFlop && state.street != Street::kTurn) {
        throw std::invalid_argument("Chance node street must be flop or turn");
      }

      auto deck = detail::RemainingDeckForChance(state, blockers);
      if (deck.empty()) {
        throw std::logic_error("No cards remaining for chance node");
      }

      std::vector<Card> dealt_cards;
      if (options.chance_samples > 0) {
        util::Rng rng(options.chance_seed ^ static_cast<std::uint64_t>(idx));
        rng.Shuffle(deck);
        const int n = std::min(options.chance_samples, static_cast<int>(deck.size()));
        dealt_cards.assign(deck.begin(), deck.begin() + n);
      } else {
        dealt_cards = std::move(deck);
      }

      children.reserve(dealt_cards.size());
      for (const auto& c : dealt_cards) {
        const GameState next = detail::AdvanceStreet(state, c);
        const int child = build(next);
        children.push_back(child);
      }
      tree.nodes[idx].children = std::move(children);
      continue;
    }

    // Decision node.
    const auto legal = LegalActions(state).actions;
    if (legal.empty()) {
      throw std::logic_error("Non-terminal decision node has no legal actions");
    }
    children.reserve(legal.size());
    for (const auto& action : legal) {
      GameState next = state;
      ApplyAction(next, action);
      const int child = build(next);
      children.push_back(child);
    }
    tree.nodes[idx].children = std::move(children);
  }

  return tree;
}

}  // namespace poker_solver::core
