#include <cassert>
#include <cmath>
#include <thread>
#include <vector>

#include "util/rng.h"
#include "util/timer.h"

namespace {

void TestRngDeterminism() {
  poker_solver::util::Rng rng1(12345);
  poker_solver::util::Rng rng2(12345);
  for (int i = 0; i < 5; ++i) {
    const int a = rng1.UniformInt(0, 100);
    const int b = rng2.UniformInt(0, 100);
    assert(a == b);
  }
}

void TestRngRange() {
  poker_solver::util::Rng rng(999);
  for (int i = 0; i < 10; ++i) {
    const double v = rng.UniformReal(0.0, 1.0);
    assert(v >= 0.0);
    assert(v < 1.0);
  }
}

void TestTimerMonotonic() {
  poker_solver::util::Timer timer;
  const double first = timer.ElapsedMillis();
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  const double second = timer.ElapsedMillis();
  assert(second >= first);
  assert(second >= 5.0 - 1.0);  // allow small scheduling variance
}

}  // namespace

int main() {
  TestRngDeterminism();
  TestRngRange();
  TestTimerMonotonic();
  return 0;
}
