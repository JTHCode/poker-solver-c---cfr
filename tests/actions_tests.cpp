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
  state.last_bet_size = 10;

  const auto result = LegalActions(state);
  assert(ContainsAction(result.actions, ActionType::kRaise, 20));  // min raise = to_call + last_bet_size
}

void TestAllInCapsShortStack() {
  GameState state{};
  state.stacks[0] = 20;
  state.stacks[1] = 100;
  state.to_call = 30;
  const auto result = LegalActions(state);
  assert(ContainsAction(result.actions, ActionType::kAllIn, 20));
  assert(!ContainsAction(result.actions, ActionType::kCall, 30));  // cannot cover full call
}

void TestApplyRaiseUpdatesPotAndToCall() {
  GameState state{};
  state.stacks[0] = 100;
  state.stacks[1] = 100;
  state.pot = 100;
  state.to_call = 10;
  state.last_bet_size = 10;

  const Action raise{ActionType::kRaise, 25};
  ApplyAction(state, raise);

  assert(state.pot == 125);          // added 25 to pot
  assert(state.stacks[0] == 75);     // paid 25
  assert(state.to_call == 15);       // opponent now has 15 to call
  assert(state.last_bet_size == 15); // min raise set to raise diff
  assert(state.current_player == 1);
}

}  // namespace

int main() {
  TestCheckAndCallLegality();
  TestMinRaiseEnforced();
  TestAllInCapsShortStack();
  TestApplyRaiseUpdatesPotAndToCall();
  return 0;
}
