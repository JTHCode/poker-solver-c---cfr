#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace poker_solver::core {

enum class Street { kPreflop, kFlop, kTurn, kRiver, kShowdown };

enum class ActionType { kFold, kCheck, kCall, kBet, kRaise, kAllIn };

struct Action {
  ActionType type;
  int amount;  // amount to commit this action (chips, integer units)
};

inline bool operator==(const Action& a, const Action& b) { return a.type == b.type && a.amount == b.amount; }
inline bool operator!=(const Action& a, const Action& b) { return !(a == b); }

struct GameState {
  Street street{Street::kPreflop};
  int pot{0};
  int to_call{0};
  int stacks[2]{0, 0};  // remaining stacks for current player (index 0) and opponent (index 1)
  int committed[2]{0, 0};  // total chips committed by each player into the pot
  int last_bet_size{0};  // size of last bet/raise increment (for min-raise enforcement)
  int current_player{0};  // 0 or 1
  bool terminal{false};
  bool street_complete{false};
  int winner{-1};  // -1 means unresolved/showdown
  int consecutive_checks{0};
};

inline int OtherPlayer(int player) { return player == 0 ? 1 : 0; }

struct LegalActionsResult {
  std::vector<Action> actions;
};

inline LegalActionsResult LegalActions(const GameState& state) {
  if (state.terminal) {
    return {};
  }
  LegalActionsResult result;

  const int stack = state.stacks[state.current_player];
  const int to_call = state.to_call;

  auto push_unique = [&](Action action) {
    for (const auto& existing : result.actions) {
      if (existing.type == action.type && existing.amount == action.amount) {
        return;
      }
    }
    result.actions.push_back(action);
  };

  if (to_call == 0) {
    push_unique({ActionType::kCheck, 0});
    if (stack > 0) {
      const int min_bet = std::max(1, state.last_bet_size > 0 ? state.last_bet_size : 1);
      const int pot_bet = std::max(min_bet, state.pot + to_call);
      const int all_in = stack;
      const int bet1 = std::min(min_bet, stack);
      const int bet2 = std::min(pot_bet, stack);
      push_unique({ActionType::kBet, bet1});
      push_unique({ActionType::kBet, bet2});
      push_unique({ActionType::kAllIn, all_in});
    }
  } else {
    push_unique({ActionType::kFold, 0});
    if (stack >= to_call) {
      push_unique({ActionType::kCall, to_call});
    }
    if (stack > 0) {
      const int min_raise = std::min(stack, to_call + std::max(state.last_bet_size, to_call));
      if (stack > to_call) {
        push_unique({ActionType::kRaise, min_raise});
        const int pot_raise = std::min(stack, state.pot + 2 * to_call);
        if (pot_raise != min_raise) {
          push_unique({ActionType::kRaise, pot_raise});
        }
      }
      push_unique({ActionType::kAllIn, stack});
    }
  }
  return result;
}

inline void ApplyAction(GameState& state, const Action& action) {
  if (state.terminal) {
    throw std::logic_error("Cannot act on terminal state");
  }
  if (state.street_complete) {
    throw std::logic_error("Cannot act after street_complete");
  }

  int& actor_stack = state.stacks[state.current_player];
  int& actor_committed = state.committed[state.current_player];

  auto switch_player = [&]() { state.current_player = OtherPlayer(state.current_player); };

  switch (action.type) {
    case ActionType::kFold:
      if (state.to_call == 0) {
        throw std::invalid_argument("Fold not allowed when to_call is 0");
      }
      state.terminal = true;
      state.winner = OtherPlayer(state.current_player);
      return;
    case ActionType::kCheck:
      if (state.to_call != 0) {
        throw std::invalid_argument("Check not allowed when to_call > 0");
      }
      state.consecutive_checks += 1;
      if (state.consecutive_checks >= 2) {
        state.street_complete = true;
        if (state.street == Street::kRiver) {
          state.terminal = true;
        }
        return;
      }
      switch_player();
      return;
    case ActionType::kCall: {
      if (action.amount != state.to_call) {
        throw std::invalid_argument("Call amount must equal to_call");
      }
      if (actor_stack < state.to_call) {
        throw std::invalid_argument("Insufficient stack to call");
      }
      actor_stack -= action.amount;
      actor_committed += action.amount;
      state.pot += action.amount;
      state.to_call = 0;
      state.consecutive_checks = 0;
      state.street_complete = true;
      if (state.street == Street::kRiver) {
        state.terminal = true;
      }
      switch_player();
      return;
    }
    case ActionType::kBet: {
      if (state.to_call != 0) {
        throw std::invalid_argument("Bet not allowed when to_call > 0");
      }
      const int bet = std::min(action.amount, actor_stack);
      if (bet <= 0) {
        throw std::invalid_argument("Bet must be positive");
      }
      actor_stack -= bet;
      actor_committed += bet;
      state.pot += bet;
      state.to_call = bet;
      state.last_bet_size = bet;
      state.consecutive_checks = 0;
      switch_player();
      return;
    }
    case ActionType::kRaise: {
      if (state.to_call <= 0) {
        throw std::invalid_argument("Raise not allowed when to_call == 0");
      }
      if (action.amount <= state.to_call) {
        throw std::invalid_argument("Raise must exceed call amount");
      }
      const int committed = std::min(action.amount, actor_stack);
      if (committed <= state.to_call) {
        throw std::invalid_argument("Raise must add chips beyond call");
      }
      actor_stack -= committed;
      actor_committed += committed;
      state.pot += committed;
      const int raise_diff = committed - state.to_call;
      state.last_bet_size = raise_diff;
      state.to_call = raise_diff;
      state.consecutive_checks = 0;
      switch_player();
      return;
    }
    case ActionType::kAllIn: {
      if (actor_stack <= 0) {
        throw std::invalid_argument("All-in with zero stack");
      }
      const int committed = actor_stack;
      actor_stack = 0;
      actor_committed += committed;
      state.pot += committed;
      if (state.to_call == 0) {
        state.to_call = committed;
        state.last_bet_size = committed;
      } else {
        // Partial or full call; opponent's new to_call is the excess.
        if (committed >= state.to_call) {
          const int raise_diff = committed - state.to_call;
          state.last_bet_size = std::max(state.last_bet_size, raise_diff);
          state.to_call = raise_diff;
        } else {
          state.to_call -= committed;
        }
      }
      state.consecutive_checks = 0;
      switch_player();
      return;
    }
  }
}

}  // namespace poker_solver::core
