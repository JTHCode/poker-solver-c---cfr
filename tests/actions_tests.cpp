#include <algorithm>
#include <cassert>
#include <vector>

#include "core/game_state.h"

namespace {

using poker_solver::core::Action;
using poker_solver::core::ActionType;
using poker_solver::core::ApplyAction;
using poker_solver::core::GameState;
using poker_solver::core::LegalActions;

bool ContainsAction(const std::vector<Action>& actions, ActionType type, int amount) {
  return std::any_of(actions.begin(), actions.end(), [&](const Action& a) {
    return a.type == type && a.amount == amount;
  });
}

void TestCheckAndCallLegality() {
  GameState state{};
  state.stacks[0] = 100;
  state.stacks[1] = 100;
  state.to_call = 0;

  const auto result = LegalActions(state);
  assert(ContainsAction(result.actions, ActionType::kCheck, 0));
  assert(!ContainsAction(result.actions, ActionType::kCall, 0));

  state.to_call = 10;
  state.street_committed[0] = 0;
  state.street_committed[1] = 10;
  state.last_bet_size = 10;
  const auto result2 = LegalActions(state);
  assert(ContainsAction(result2.actions, ActionType::kFold, 0));
  assert(ContainsAction(result2.actions, ActionType::kCall, 10));
}

void TestMinRaiseEnforced() {
  GameState state{};
  state.stacks[0] = 50;
  state.stacks[1] = 50;
  state.pot = 100;
  state.to_call = 10;
  state.street_committed[0] = 0;
  state.street_committed[1] = 10;
  state.last_bet_size = 10;

  const auto result = LegalActions(state);
  // Abstraction raises are >= NLHE minimum raise, but may not include the exact min size.
  assert(ContainsAction(result.actions, ActionType::kRaise, 25));  // 2.5x raise-to over a 10-unit bet
}

void TestAllInCapsShortStack() {
  GameState state{};
  state.stacks[0] = 20;
  state.stacks[1] = 100;
  state.to_call = 30;
  state.street_committed[0] = 0;
  state.street_committed[1] = 30;
  state.last_bet_size = 30;
  const auto result = LegalActions(state);
  assert(ContainsAction(result.actions, ActionType::kAllIn, 20));
  assert(!ContainsAction(result.actions, ActionType::kCall, 30));  // cannot cover full call
}

void TestBetSizesOnNewStreet() {
  GameState state{};
  state.pot = 30;
  state.stacks[0] = 100;
  state.stacks[1] = 100;
  state.to_call = 0;

  const auto result = LegalActions(state);
  assert(ContainsAction(result.actions, ActionType::kCheck, 0));
  assert(ContainsAction(result.actions, ActionType::kBet, 10));  // 1/3 pot
  assert(ContainsAction(result.actions, ActionType::kBet, 20));  // 2/3 pot
  assert(ContainsAction(result.actions, ActionType::kBet, 30));  // pot
  assert(ContainsAction(result.actions, ActionType::kAllIn, 100));
}

void TestCheckCheckCompletesStreet() {
  GameState state{};
  state.street = poker_solver::core::Street::kFlop;
  state.stacks[0] = 100;
  state.stacks[1] = 100;
  state.to_call = 0;
  state.current_player = 0;

  ApplyAction(state, Action{ActionType::kCheck, 0});
  assert(state.current_player == 1);
  assert(!state.street_complete);
  assert(!state.terminal);

  ApplyAction(state, Action{ActionType::kCheck, 0});
  assert(state.street_complete);
  assert(!state.terminal);  // terminal only on river
}

void TestApplyRaiseUpdatesPotAndToCall() {
  GameState state{};
  state.stacks[0] = 100;
  state.stacks[1] = 100;
  state.pot = 100;
  state.to_call = 10;
  state.street_committed[0] = 0;
  state.street_committed[1] = 10;
  state.last_bet_size = 10;

  const Action raise{ActionType::kRaise, 25};
  ApplyAction(state, raise);

  assert(state.pot == 125);          // added 25 to pot
  assert(state.stacks[0] == 75);     // paid 25
  assert(state.to_call == 15);       // opponent now has 15 to call
  assert(state.last_bet_size == 15); // min raise set to raise diff
  assert(state.raises_this_street == 1);
  assert(state.reopen_allowed);
  assert(state.current_player == 1);
}

void TestMaxRaisesCapDisallowsFurtherRaises() {
  GameState state{};
  state.pot = 100;
  state.stacks[0] = 100;
  state.stacks[1] = 100;
  state.to_call = 10;
  state.street_committed[0] = 0;
  state.street_committed[1] = 10;
  state.last_bet_size = 10;
  state.raises_this_street = 3;
  state.reopen_allowed = true;

  const auto result = LegalActions(state);
  assert(ContainsAction(result.actions, ActionType::kFold, 0));
  assert(ContainsAction(result.actions, ActionType::kCall, 10));
  // Cap means no additional raises, including all-in raises.
  assert(!ContainsAction(result.actions, ActionType::kAllIn, 100));
  for (const auto& a : result.actions) {
    assert(a.type != ActionType::kRaise);
  }
}

void TestShortAllInRaiseDoesNotReopen() {
  GameState state{};
  state.pot = 100;
  state.stacks[0] = 15;
  state.stacks[1] = 100;
  state.to_call = 10;
  state.street_committed[0] = 0;
  state.street_committed[1] = 10;
  state.last_bet_size = 10;
  state.current_player = 0;

  ApplyAction(state, Action{ActionType::kAllIn, 15});

  assert(!state.terminal);
  assert(!state.street_complete);
  assert(state.current_player == 1);
  assert(state.to_call == 5);
  assert(!state.reopen_allowed);
  assert(state.raises_this_street == 0);

  const auto result = LegalActions(state);
  assert(ContainsAction(result.actions, ActionType::kFold, 0));
  assert(ContainsAction(result.actions, ActionType::kCall, 5));
  for (const auto& a : result.actions) {
    assert(a.type != ActionType::kRaise);
    assert(a.type != ActionType::kAllIn);  // all-in would be a raise here and should be disallowed
  }
}

void TestShortAllInCallEndsHand() {
  GameState state{};
  state.pot = 100;
  state.stacks[0] = 5;
  state.stacks[1] = 100;
  state.to_call = 10;
  state.street_committed[0] = 0;
  state.street_committed[1] = 10;
  state.last_bet_size = 10;
  state.current_player = 0;

  ApplyAction(state, Action{ActionType::kAllIn, 5});

  assert(state.terminal);
  assert(state.street_complete);
  assert(state.winner == -1);
  assert(state.to_call == 0);
  assert(state.pot == 105);
}

}  // namespace

int main() {
  TestCheckAndCallLegality();
  TestMinRaiseEnforced();
  TestAllInCapsShortStack();
  TestBetSizesOnNewStreet();
  TestCheckCheckCompletesStreet();
  TestApplyRaiseUpdatesPotAndToCall();
  TestMaxRaisesCapDisallowsFurtherRaises();
  TestShortAllInRaiseDoesNotReopen();
  TestShortAllInCallEndsHand();
  return 0;
}
