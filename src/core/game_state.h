#pragma once

#include <algorithm>
#include <stdexcept>
#include <vector>

#include "core/board.h"

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
  Board board{};
  int board_count{0};  // number of known board cards in order flop(3),turn,river
  int pot{0};
  int to_call{0};
  int stacks[2]{0, 0};  // remaining stacks for current player (index 0) and opponent (index 1)
  int committed[2]{0, 0};  // total chips committed by each player into the pot
  int street_committed[2]{0, 0};  // chips committed by each player on the current street
  int last_bet_size{0};  // size of last bet/raise increment (for min-raise enforcement)
  int raises_this_street{0};  // number of raises after the first bet on this street (abstraction cap)
  int current_player{0};  // 0 or 1
  bool terminal{false};
  bool street_complete{false};
  bool reopen_allowed{true};  // simplified: false after short all-in raise; next player may not raise
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
  if (state.street_complete) {
    return {};
  }
  LegalActionsResult result;

  const int stack = state.stacks[state.current_player];
  const int to_call = state.to_call;

  static constexpr int kMinBet = 1;
  static constexpr int kMaxRaisesPerStreet = 3;  // corresponds to bet + up to 3 raises ("4-bet cap")

  struct RaiseMult {
    int num;
    int den;
  };
  static constexpr RaiseMult kRaiseMults[] = {{5, 2}, {13, 4}, {4, 1}};  // 2.5x, 3.25x, 4x

  auto opponent_bet_to = [&]() { return state.street_committed[OtherPlayer(state.current_player)]; };

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
      // Abstraction: 33%, 66%, pot, all-in.
      const int pot = std::max(0, state.pot);
      const int bet33 = std::max(kMinBet, pot / 3);
      const int bet66 = std::max(kMinBet, (pot * 2) / 3);
      const int betpot = std::max(kMinBet, pot);

      const int candidates[] = {bet33, bet66, betpot};
      for (int bet_to : candidates) {
        if (bet_to <= 0) {
          continue;
        }
        if (bet_to >= stack) {
          continue;  // represent stack-sized bet as all-in only
        }
        push_unique({ActionType::kBet, bet_to});
      }
      push_unique({ActionType::kAllIn, stack});
    }
  } else {
    push_unique({ActionType::kFold, 0});
    if (stack >= to_call) {
      push_unique({ActionType::kCall, to_call});
    }
    if (stack <= 0) {
      return result;
    }

    // Always allow all-in as a call when short.
    if (stack < to_call) {
      push_unique({ActionType::kAllIn, stack});
      return result;
    }

    const bool raises_remaining = state.raises_this_street < kMaxRaisesPerStreet;
    const bool raises_allowed = state.reopen_allowed && raises_remaining;

    if (stack > to_call && raises_allowed) {
      const int opp_to = opponent_bet_to();
      const int min_raise_to = opp_to + std::max(state.last_bet_size, kMinBet);
      const int actor_street = state.street_committed[state.current_player];

      for (const auto& mult : kRaiseMults) {
        const int raise_to = (opp_to * mult.num) / mult.den;
        if (raise_to < min_raise_to) {
          continue;
        }
        const int commit = raise_to - actor_street;
        if (commit <= to_call) {
          continue;
        }
        if (commit >= stack) {
          continue;  // represent stack-sized raise as all-in only
        }
        push_unique({ActionType::kRaise, commit});
      }

      // All-in raise (counts toward cap/reopen).
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
  int& actor_street = state.street_committed[state.current_player];
  const int other = OtherPlayer(state.current_player);
  int& other_street = state.street_committed[other];

  auto update_to_call = [&]() {
    const int opp = OtherPlayer(state.current_player);
    const int diff = state.street_committed[opp] - state.street_committed[state.current_player];
    state.to_call = std::max(0, diff);
  };

  auto switch_player = [&]() {
    state.current_player = OtherPlayer(state.current_player);
    update_to_call();
  };

  auto maybe_end_all_in = [&]() {
    if ((state.stacks[0] == 0 || state.stacks[1] == 0) && state.to_call == 0 && !state.terminal) {
      state.street_complete = true;
      state.terminal = true;
      state.winner = -1;
    }
  };

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
      actor_street += action.amount;
      state.pot += action.amount;
      state.to_call = 0;
      state.consecutive_checks = 0;
      state.street_complete = true;
      if (state.street == Street::kRiver) {
        state.terminal = true;
      }
      switch_player();
      maybe_end_all_in();
      return;
    }
    case ActionType::kBet: {
      if (state.to_call != 0) {
        throw std::invalid_argument("Bet not allowed when to_call > 0");
      }
      if (action.amount <= 0) {
        throw std::invalid_argument("Bet must be positive");
      }
      if (action.amount > actor_stack) {
        throw std::invalid_argument("Bet exceeds stack");
      }
      const int bet = action.amount;
      actor_stack -= bet;
      actor_committed += bet;
      actor_street += bet;
      state.pot += bet;
      state.last_bet_size = bet;
      state.raises_this_street = 0;
      state.reopen_allowed = true;
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
      if (action.amount > actor_stack) {
        throw std::invalid_argument("Raise exceeds stack");
      }
      const int old_opp_to = other_street;
      const int committed_now = action.amount;
      actor_stack -= committed_now;
      actor_committed += committed_now;
      actor_street += committed_now;
      state.pot += committed_now;
      const int raise_size = actor_street - old_opp_to;
      state.last_bet_size = raise_size;
      state.raises_this_street += 1;
      state.reopen_allowed = true;
      state.consecutive_checks = 0;
      switch_player();
      maybe_end_all_in();
      return;
    }
    case ActionType::kAllIn: {
      if (actor_stack <= 0) {
        throw std::invalid_argument("All-in with zero stack");
      }
      const int committed_now = actor_stack;
      actor_stack = 0;
      const int old_to_call = state.to_call;
      const int old_opp_to = other_street;

      actor_committed += committed_now;
      actor_street += committed_now;
      state.pot += committed_now;

      if (old_to_call == 0) {
        // All-in bet.
        state.last_bet_size = committed_now;
        state.raises_this_street = 0;
        state.reopen_allowed = true;
        state.consecutive_checks = 0;
        switch_player();
        maybe_end_all_in();
        return;
      }

      if (committed_now < old_to_call) {
        // Short all-in call: betting immediately ends (simplified).
        state.to_call = 0;
        state.street_complete = true;
        state.terminal = true;
        state.winner = -1;
        state.consecutive_checks = 0;
        return;
      }

      const int raise_size = actor_street - old_opp_to;
      if (raise_size >= std::max(state.last_bet_size, 1)) {
        state.last_bet_size = raise_size;
        state.raises_this_street += 1;
        state.reopen_allowed = true;
      } else {
        // Short all-in raise does not reopen action for the bettor (simplified HU rule).
        state.reopen_allowed = false;
      }
      state.consecutive_checks = 0;
      switch_player();
      maybe_end_all_in();
      return;
    }
  }
}

}  // namespace poker_solver::core
