#include <cassert>
#include <cmath>

#include "solver/kuhn_cfr.h"

namespace {

using poker_solver::solver::KuhnCfrTrainer;

void AssertNear(double value, double expected, double tol) {
  assert(std::fabs(value - expected) <= tol);
}

// Convergence sanity checks against Kuhn poker equilibrium structure.
// Kuhn has a family of equilibria; CFR can converge to different points in that set.
// These checks validate key relationships and response behaviors.
void TestKuhnConvergence() {
  KuhnCfrTrainer trainer;
  trainer.Train(40000);

  const double tol = 0.08;  // keep loose for deterministic CFR with finite iterations.

  const auto root_j = trainer.AverageStrategy("J:");
  const auto root_q = trainer.AverageStrategy("Q:");
  const auto root_k = trainer.AverageStrategy("K:");
  const auto resp_j = trainer.AverageStrategy("J:b");  // call vs fold
  const auto resp_q = trainer.AverageStrategy("Q:b");
  const auto resp_k = trainer.AverageStrategy("K:b");

  // Root: Q rarely bets; K bets frequently; J bluff rate tracks K value-bet rate (~K/3).
  assert(root_q[1] <= 0.05);
  assert(root_k[1] >= 0.60);
  AssertNear(root_j[1], root_k[1] / 3.0, 0.06);

  // Response to a bet: J mostly folds, K mostly calls, Q calls around 1/3.
  assert(resp_j[0] <= 0.05);
  assert(resp_k[0] >= 0.95);
  AssertNear(resp_q[0], 1.0 / 3.0, tol);
}

}  // namespace

int main() {
  TestKuhnConvergence();
  return 0;
}
