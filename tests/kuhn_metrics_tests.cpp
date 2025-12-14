#include <cassert>

#include "solver/kuhn_cfr.h"
#include "solver/kuhn_metrics.h"

namespace {

using poker_solver::solver::KuhnCfrTrainer;
using poker_solver::solver::KuhnExploitability;

void TestKuhnExploitabilityDecreasesWithIterations() {
  KuhnCfrTrainer a;
  a.Train(200);
  const auto ea = KuhnExploitability(a);

  KuhnCfrTrainer b;
  b.Train(20000);
  const auto eb = KuhnExploitability(b);

  assert(ea.exploitability > 0.0);
  assert(eb.exploitability > 0.0);
  assert(eb.exploitability < ea.exploitability);
  assert(eb.exploitability < 0.05);
}

}  // namespace

int main() {
  TestKuhnExploitabilityDecreasesWithIterations();
  return 0;
}
