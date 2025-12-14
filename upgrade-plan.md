# Upgrade Plan (Pre-CFR+ Readiness)

This document is the implementation checklist to take this project from its current “toy” NLHE scaffolding (stubbed showdown, simplified tree) to a **correct, reproducible, and benchmarkable** solver where switching from CFR → CFR+ would be a meaningful, measurable improvement.

Scope boundaries:
- This plan **does not implement CFR+**.
- This plan focuses on **correct payoff evaluation**, **correct betting/tree modeling for the chosen abstraction**, **objective quality metrics**, and **repeatable benchmarks**.
- Avoid refactors unless required for correctness, performance, or testability.

Primary environment: **WSL/Linux (Bash), CMake, C++17**.

## Project Standards (Must-Follow)

- C++17 (no C++20-only syntax in production or tests).
- Naming: types `CamelCase`, functions/variables `snake_case`, constants `kPascalCase`.
- Determinism: all stochastic components must accept explicit seeds; tests must be deterministic.
- Performance hygiene:
  - avoid unchecked dynamic allocations in hot paths
  - preallocate/reserve in solver loops where possible
  - avoid string-heavy keys in inner loops (prefer integer IDs) once correctness is locked
- Safety: validate inputs; fail fast with actionable error messages; never silently produce invalid strategies.
- Documentation: update `changelog.md` after each coding session; update this `upgrade-plan.md` as phases complete/requirements shift.

## Definitions (What “Respectable” Means Here)

“Respectable” means:
- Correct hand ranking and terminal payoffs (including ties/splits).
- A well-defined betting abstraction and legal-action engine that matches it.
- Chance handling (board runout) that is correct (exact or controlled sampling).
- Objective metrics showing strategy quality improves with iterations (exploitability/best-response), and performance is measured with a repeatable benchmark.

## Phases Overview

- Phase A: Baseline instrumentation and benchmarking
- Phase B: Hand evaluation and terminal resolution
- Phase C: NLHE rules + betting abstraction correctness
- Phase D: Tree generation + info-set/keying discipline
- Phase E: Solver quality metrics (best-response/exploitability)
- Phase F: Performance readiness and reproducible benchmarking gates
- Final gates: “CFR+ readiness” criteria

---

## Phase A — Baseline Instrumentation and Benchmark Harness

Goal: Make performance and quality observable without changing solver behavior meaningfully.

- [ ] Add a standardized benchmark configuration (documented defaults) for:
  - number of situations
  - iterations
  - branch threshold
  - fixed seed
- [ ] Extend JSONL `metrics` to include:
  - solve wall time per spot (already present)
  - nodes visited / decisions made per solve (approx is acceptable initially)
  - number of branch solves executed
  - number of chance samples (if sampling is used)
- [ ] Ensure `scripts/bench.sh` prints:
  - situations/min (already)
  - p50/p90/p99 solve time (already)
  - branches stats (already)
  - optional: nodes/sec when available
- [ ] Add a “benchmark contract” section to `tests/READ_ME.md`:
  - exact commands to reproduce benchmark results
  - recommended Release build usage

Exit criteria:
- You can run `./scripts/bench.sh` and get stable-ish throughput numbers for a fixed seed/config.

---

## Phase B — Hand Evaluator + Terminal Payoff Correctness

Goal: Replace `showdown_winner` stubs with correct hold’em evaluation and correct payoff math.

### B1: Hand evaluator implementation

Choose one approach:
- Implement a fast 7-card evaluator (rank categories + kicker ordering).

- [ ] Add `src/core/hand_rank.h` (or similar) defining:
  - `HandRank` value type with total ordering
  - `EvaluateHoldem7(hole[2], board[5]) -> HandRank`
- [ ] Add unit tests `tests/hand_rank_tests.cpp` covering:
  - each category (high card .. straight flush)
  - ties and split scenarios
  - known tricky cases (wheel straight, board pairs, etc.)

### B2: Terminal utility + pot distribution

- [ ] Replace terminal payoff stubs in solver path with:
  - fold payoff (already conceptually present)
  - showdown payoff using evaluator:
    - win: +opponent_committed
    - loss: -self_committed
    - tie: split pot (define rounding rule for odd chips; for heads-up, decide who gets remainder—document it)
- [ ] Add tests for terminal resolution correctness:
  - fold → correct chipflow
  - showdown win/loss/tie → correct chipflow

### B3: All-in before river (chance resolution)

Decide mode(s):
- Exact: enumerate remaining board runouts (only feasible for small remaining cards; can be used in tests).
- Sampling: Monte Carlo with explicit `--board_samples`, seeded RNG, and metrics reporting.

- [ ] Implement “runout handling” for terminal all-ins:
  - if all-in and board incomplete: compute expected value via exact or sampling
- [ ] Add deterministic tests:
  - small exact enumeration test case (fixed blockers, small remaining runout space)
  - sampling reproducibility test (fixed seed gives stable EV within tolerance)

Exit criteria:
- Hand evaluator passes tests.
- Terminal utility tests pass including ties.
- All-in before river produces deterministic results (exact or seeded sampling).

---

## Phase C — NLHE Rules + Betting Abstraction Correctness

Goal: Define and enforce the betting model used by the solver. The solver can be “abstracted NLHE” but must be internally consistent and correct for that abstraction.

### C1: Define the abstraction (write it down)

- [ ] Decide and document (decision are in `details.md`):
  - stack size model (bb, units)
  - streets modeled (flop/turn/river only or include preflop)
  - bet sizes allowed per street (e.g., 33%, 66%, pot, all-in)
  - raise sizes allowed (e.g., 2.5x, 3.5x, all-in)
  - max raises per street (if any)
  - “reopen action” handling for short all-ins (exact or simplified rule)

### C2: Action legality engine (authoritative)

- [ ] Implement/upgrade `core/game_state.h` legality rules to match the chosen abstraction:
  - min-bet/min-raise
  - raise-to vs raise-by correctness
  - all-in caps and short-stack edge cases
  - street completion rules
  - consistent state transitions (pot/stacks/to_call/last_bet_size)
- [ ] Unit tests:
  - min-raise enforcement
  - cap at all-in
  - check/call symmetry and street transitions
  - reopen behavior if supported

Exit criteria:
- The legality engine is the single source of truth for betting rules.
- Unit tests cover the edge cases above.

---

## Phase D — Tree Generation + Chance Nodes + Info-Set Discipline

Goal: Generate a correct decision tree for the abstraction and ensure info-set keys are stable and appropriate.

### D1: Tree generation

- [ ] Add a tree builder that:
  - expands decision nodes using the legality engine
  - expands chance nodes (deal remaining board cards) based on game state
  - marks terminal nodes with resolved utility paths
- [ ] Ensure tree generation is deterministic for fixed seed/config when sampling is used.

### D2: Info-set keying

Current keys are string-based and minimal. Before performance tuning, ensure correctness:

- [ ] Define what belongs in an info-set key for the abstraction:
  - street
  - public board cards or board bucket
  - action history abstraction (or exact sequence if small)
  - position / player-to-act
  - any relevant stack/pot buckets if you’re abstracting them
- [ ] Implement a stable key representation:
  - initially string is OK; plan to migrate to integer IDs later for speed
- [ ] Tests:
  - deterministic keys for identical states
  - distinct keys for distinct observable states

Exit criteria:
- Tree nodes and chance nodes correspond to valid states.
- Info-set keys are deterministic and match the chosen abstraction.

---

## Phase E — Objective Quality Metrics (Best-Response / Exploitability)

Goal: Make solver quality measurable so CFR+ improvements can be evaluated objectively.

### E1: Best-response evaluator for small trees

- [ ] Implement best-response for the opponent on a small tree:
  - exact traversal using the same tree representation
  - compute exploitability (or at least “BR value vs current strategy”)
- [ ] Tests:
  - Kuhn: exploitability decreases as iterations increase (monotonic not required; overall decreasing trend)
  - “River-only holdem abstraction”: exploitability decreases with iterations

### E2: Convergence tracking

- [ ] Add metric output:
  - exploitability estimate at intervals (e.g., every N iterations)
  - or BR value vs current strategy

Exit criteria:
- For a fixed benchmark config, quality metrics improve as iterations increase.
- You can compare variants (CFR vs future CFR+) using the same metric.

---

## Phase F — Performance Readiness and Benchmark Gates

Goal: Ensure performance measurements are meaningful and that bottlenecks are understood before switching to CFR+.

- [ ] Ensure Release builds are the default for benchmarks (`build_rel`).
- [ ] Reduce avoidable overhead in hot loops:
  - avoid string key building per node visit (move to integer info-set IDs if needed)
  - preallocate vectors (regrets/strategy) where sizes are known
  - avoid repeated legality computation if state/action sets are cached in the tree
- [ ] Add profiling hooks (optional):
  - `--profile` to dump counts/timers
- [ ] Establish baseline benchmark numbers on your machine:
  - situations/min at iterations = {500, 2000, 10000}
  - exploitability/quality metric at those settings

Exit criteria:
- Benchmarks are repeatable and quality metrics are included.
- “Time per iteration” is stable enough to compare algorithm variants.

---

## Final Gates — CFR+ Readiness Checklist (Must Pass All)

### Gate 1: Correctness
- [ ] Hand evaluator passes comprehensive unit tests (all categories + ties).
- [ ] Terminal payoffs (fold/showdown/tie) are correct and tested.
- [ ] All-in before river resolution is correct (exact test) and deterministic (sampling seeded).

### Gate 2: Model Integrity
- [ ] Betting legality matches the documented abstraction and has edge-case tests.
- [ ] Tree generation produces only legal states and terminates correctly.
- [ ] Info-set key definition is documented and enforced by tests.

### Gate 3: Objective Quality
- [ ] Best-response/exploitability metric exists for at least one NLHE-derived small abstraction.
- [ ] Quality metric improves with more iterations on fixed seed/config.
- [ ] Output strategies are valid distributions (sum to ~1) and checked by tests.

### Gate 4: Benchmarkability
- [ ] `scripts/bench.sh` runs from a clean repo and reports:
  - situations/min
  - solve time distribution
  - branch counts
  - quality metric (exploitability/BR) for a standard benchmark config
- [ ] Baseline results are recorded (in `details.md` or a new `benchmarks.md`) for CFR.

### Gate 5: “CFR+ will matter” condition
- [ ] The solver is solving a non-trivial tree where exploitability is meaningful (not stubbed payoffs).
- [ ] Iteration count is a dominant cost (solver is iteration-bound rather than string/IO-bound).
- [ ] You have a defined target quality level (exploitability threshold or BR gap) that CFR struggles to reach within budget, making CFR+ iteration-reduction valuable.

When all gates pass, implementing CFR+ becomes a measurable optimization rather than a speculative refactor.

---

## Notes on Implementation Order (Recommended)

1) Phase B (evaluator + terminal correctness) is the most important foundation.
2) Phase C/D (rules + tree + info-sets) makes “the game” well-defined.
3) Phase E (quality metrics) makes CFR vs CFR+ comparable.
4) Phase F (performance readiness) prevents misleading speed comparisons.

