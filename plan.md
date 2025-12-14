# Development Plan and Architecture

## Plan Overview
- Build in phases: utilities → domain/ranges → game model/tree → CFR core → solver pipeline → JSONL writer → CLI/progress → validation/perf.
- Keep modules small and testable; deterministic seeding for reproducible tests and benchmarks.

## Proposed Folder Structure
- `src/`
  - `cli/` (arg parsing, progress, orchestration)
  - `core/` (`cards`, `ranges`, `game_state`, `tree`, `actions`, `betting`, `board_gen`)
  - `solver/` (`cfr`, `strategy`, `subtree_expansion`, `rollout`)
  - `io/` (`jsonl_writer`, `preflop_loader`, `config`)
  - `util/` (`rng`, `logging`, `timer`, `metrics`)
- `tests/` mirroring `src/` layout
- `data/preflop-ranges/` (given)
- `scripts/` (benchmarks, sample runs)
- `build/` (artifacts, cmake cache, etc.)

## Key Modules and Responsibilities
- `cards`: card representation, deck, hand ranking utilities (only as needed for equity eval).
- `ranges`: load preflop ranges JSON into dense structures (e.g., 169/1326 combo probs); sampling and normalization.
- `game_state`: pot, stacks, bets, positions (UTG, HJ, CO, BTN, SB, BB), street, board, action history.
- `actions`: enums for action types; validates legal actions from state and allowed bet sizes/raises.
- `betting`: computes pot sizes, legal bet amounts (1/3, 2/3, pot, all-in) and raises (2.5x, 3.5x, all-in), clamps to stack.
- `board_gen`: random board generation without collisions with hole cards.
- `tree`: game tree node structure, children, info-set keys, ownership (hero/villain), terminal detection.
- `cfr`: core CFR loop (regret matching, strategy accumulation, average strategy); supports chance nodes for board cards; configurable iterations.
- `strategy`: storage for regrets and strategy sums; serialization to output-friendly JSON objects.
- `subtree_expansion`: after root strategy, expand/solve only hero actions with frequency ≥ 20%; villain assumed optimal everywhere; prune < 20% hero branches.
- `rollout`: solves from given node to river using CFR with villain optimal policy; reuse caches to avoid recomputation.
- `preflop_loader`: load `/preflop-ranges` files; map positions to range distributions.
- `jsonl_writer`: append-only writer; stream open with `std::ios::app`; each spot → single line JSON; flush per write.
- `cli`: parse `number_of_situations`, set seed, loop: generate scenario, solve, append JSON; show progress (count, elapsed).
- `logging/timer/metrics`: simple scoped timer, per-iteration metrics for performance tests.

## Data Structures (Conceptual)
- `Card { uint8_t rank, suit; }`
- `Board { Card flop[3]; Card turn; Card river; }`
- `Range { std::array<float, 1326> combo_probs; }` with position tag.
- `Action { enum Type { Fold, Call, Bet, Raise, AllIn, Check }; double amount; }`
- `Node { StateKey info_set; std::vector<Action> actions; std::vector<NodeId> children; double terminal_value[2]; bool terminal; bool chance; }`
- `GameState { Street street; double pot; double to_call; double stacks[2]; Pos hero, villain; Board board; History history; }`
- `RegretTable { std::vector<double> regret; std::vector<double> strategy_sum; }` keyed by info-set and action index.
- `StrategySnapshot { map<InfoSet, vector<double>> avg_strategy; }`
- `SolvedSpot { metadata, root_strategy, sub_branches, ranges }` (JSON object per line).

## CFR Algorithm Outline
- Use vanilla CFR (not CFR+ initially) for simplicity; option to swap CFR+ later.
- Info-set key: (street, board texture bucket if needed, action history abstraction, position, hole-card bucket if using abstraction). First version can keep explicit hole cards (bucketing later if needed).
- At each node: compute strategy via regret matching on positive regrets; traverse action set (fixed bet sizes); accumulate regrets and strategy sums.
- Chance handling: preflop hole cards from ranges, board dealt as chance nodes; for offline solver, prefer external sampling (Monte Carlo) to avoid exploding tree; configurable number of board samples.
- Termination: showdown or everyone all-in; compute utility with hand evaluator.
- Average strategy extraction after iterations; root policy used to pick hero branches with frequency ≥ 20%.
- Sub-branch solving: for each hero action ≥ 20% at root, lock that action, run CFR for continuation assuming villain optimal; prune others.

## Scenario Generation
- Randomly pick hero/villain positions (ordered, always heads-up).
- Sample preflop action line given open/3-bet sizes (2.5bb open, 10bb 3-bet) ensuring stack validity.
- Sample hole cards from respective ranges; ensure no collision.
- Generate random board consistent with remaining deck.
- Build initial game state from sampled preflop history.

## JSONL Output Schema (Per Line)
- `metadata`: id, seed, hero_pos, villain_pos, stacks, preflop_action_line, bet_sizes, raise_sizes, iterations, timestamp, version.
- `ranges`: hero_range (input), villain_range (input).
- `root_strategy`: map info_set → action probabilities at root street.
- `solved_branches`: only hero branches ≥ 20% with solved continuation strategies, plus villain optimal policies.
- `board`: flop/turn/river cards.
- `metrics`: solve_time_ms, nodes_touched, samples.

## Efficient JSONL Append
- Use `std::ofstream out(path, std::ios::app);` keep open for batch or open/close per write if safer.
- Append newline after each JSON object; flush per write to reduce loss on crash.
- Avoid large in-memory buffers; serialize spot-by-spot.

## Testing Strategy
- Unit tests:
  - `cards`: parsing, collision checks.
  - `ranges`: load/normalize, sampling respects weights.
  - `betting/actions`: legal action generation, pot/stack updates, cap at all-in.
  - `board_gen`: uniqueness, excludes hole cards.
  - `jsonl_writer`: appends well-formed lines, no truncation.
  - `cfr`: Kuhn poker toy game convergence; regret non-negativity; average strategy sums to 1.
  - `tree/game_state`: terminal detection, to_call correctness.
- Integration tests:
  - End-to-end small iteration solve (tiny game) producing valid JSONL line.
  - Deterministic run with fixed seed yields same metadata/root strategy shape (float tolerance).
- Performance/benchmarks:
  - Iterations per second on a fixed node set.
  - Memory footprint of regret tables versus node count.
- Validation:
  - Root strategy probabilities sum to about 1 per info-set.
  - No negative stack or pot.
  - Branch pruning respects 20% threshold (assert).
  - JSONL lines parse and schema fields present.

## Phased Build Order (Checklist)
- [x] 1) Project skeleton and build system and CLI scaffold (no logic): set up CMake/toolchain, add top-level targets, create empty folders per structure, stub `main` that parses `--help` and `number_of_situations`, wire basic logging macro, and add initial CI/build script if used.  
  - **Dev Notes:** Created CMake scaffold with `poker_solver_cli`, interface include path, and warning flags; CLI stub parses args and logs placeholder messages; logging macro available in `util/logging.h`.
  
- [x] 2) Utilities: RNG, timer, logging: implement seeded RNG wrapper (std::mt19937_64), scoped timer utility, simple logger with levels; unit tests verifying deterministic seeding and timing wrapper behavior.  
  - **Dev Notes:** Added `util/rng.h` (deterministic mt19937_64 wrapper) and `util/timer.h` (steady_clock timer). Unit tests in `tests/util_tests.cpp` cover RNG determinism/range and timer monotonicity; ensure toolchain available before running.
  
- [x] 3) Cards/board utilities and board generator: implement card representation, deck shuffling/drawing, collision checks; board generator that draws flop/turn/river from remaining deck; tests for uniqueness and exclusion of known cards.  
  - **Dev Notes:** Added `core/cards.h`, `core/board.h`, and `core/board_gen.h`; board generation removes blockers, shuffles deck with `util::Rng`, and deals flop/turn/river. Tests in `tests/board_tests.cpp` cover deck uniqueness, deterministic boards via seed, blocker exclusion, and duplicate-blocker error.
  
- [X] 4) Ranges loader and validation: implement JSON loader for `/preflop-ranges`, normalization, and sampling API (weighted combo sampling); tests for file parse, normalization sums, and collision-free sampling with provided blockers.  
  - **Dev Notes:** 12/13/2025: Added `io/preflop_range.h` loader using the root `preflop-ranges/` JSON files, normalizing combo weights and providing blocker-aware sampling; tests cover parse/normalization and blocker exclusion.
  
- [X] 5) Game state, actions, betting rules; unit tests on legality and stack math: encode positions, streets, pot/stacks/to-call tracking; action validation (fold/call/check/bet/raise/all-in) with allowed bet/raise sizes; pot updates and stack deductions; tests for edge cases (all-in caps, min-raise enforcement, transition check/call symmetry).  
  - **Dev Notes:** 12/13/2025: Added `core/game_state.h` with basic streets, action types, legal action generation, min-raise enforcement, and blocker-aware stack/pot updates including partial all-ins; tests (`actions_tests`) cover check/call symmetry, min-raise, short-stack all-in caps, and raise application.
  
- [X] 6) Tree structures and info-set keying: define node representation, child linkage, terminal flags, and info-set key builder using state abstraction (street, position, action history, hole-card/board buckets as chosen); tests for deterministic keys and terminal detection.  
  - **Dev Notes:** 12/13/2025: Added `core/tree.h` with simple node representation, owners, and deterministic info-set key derived from game state and owner; tests (`tree_tests`) verify deterministic keys, owner separation, and terminal flag inclusion.
  
- [X] 7) CFR core on toy game; pass Kuhn test; add hand evaluator stub or mocked values: implement regret matching, strategy accumulation, traversal; run on Kuhn poker to verify convergence toward Nash; mock evaluator returning fixed utilities to keep small game testable.  
  - **Dev Notes:** 12/13/2025: Added a minimal Kuhn poker CFR trainer (`solver/kuhn_cfr.h`) with regret matching and average strategy extraction; added `cfr_tests` validating Kuhn equilibrium structure (K value-bet high, Q rarely bets, J bluff tracks K/3, and correct call/fold behavior vs a bet).
  
- [X] 8) Integrate NLHE betting model into CFR; fixed bet/raise sizes; terminal handling: plug real action generator from step 5, connect hand evaluator stub, ensure showdown/all-in resolution; add tests on tiny trees (single street) for payoff correctness.  
  - **Dev Notes:** 12/13/2025: Added `solver/nlhe_cfr.h` integrating CFR traversal with `core::LegalActions`/`ApplyAction` (fixed action sizes) and a stubbed showdown winner; extended `GameState` to track committed chips and street completion for correct fold/showdown payoff math; added `nlhe_cfr_tests` to validate fold-vs-call dominance on a tiny river tree with deterministic showdown outcomes.
  
- [X] 9) Scenario generator: positions, preflop action line, board sampling: random hero/villain position selection, sample preflop actions (2.5bb open, 10bb 3-bet) respecting stacks, sample hole cards from ranges, build initial game state with history; tests for validity and no collisions.  
  - **Dev Notes:** 12/13/2025: Added `solver/scenario_generator.h` to sample opener/defender positions, preflop line (open-call vs open-3bet-call), hole cards from preflop ranges with collision blockers, and a blocked random board; constructs a deterministic initial flop `GameState` with committed/pot/stacks set and includes a `preflop_action_line` string. Added `scenario_tests` for determinism, pot/stack validity, and collision-free cards/board.
  
- [X] 10) JSONL writer and schema validation test: implement append-only writer, schema builder for metadata/root strategy/branches/ranges/metrics, unit test that writes multiple lines and parses back cleanly.  
  - **Dev Notes:** 12/13/2025: Added `io/jsonl_writer.h` (append-only, newline-enforced) plus a minimal schema builder `io/spot_json.h` producing a single-line JSON object with `metadata`, `ranges`, `root_strategy`, `solved_branches`, `board`, `metrics`. Added a tiny JSON parser `io/minijson.h` for unit testing and `jsonl_writer_tests` to write two lines then parse/validate required keys.
  
- [ ] 11) Sub-branch solver logic (>= 20% hero actions) and villain-optimal assumption; integrate with CFR runs: after root solve, identify hero actions >= 20% frequency, lock choice, rerun CFR for continuations with villain optimal everywhere; tests to ensure pruning respects threshold and locked action is enforced.  
  - **Dev Notes:** _Add observations, setup quirks, or reminders relevant to this step._
  
- [ ] 12) CLI loop: generate N situations, solve, append, show progress; add elapsed time display: wire generator + solver + writer; progress reporting per X spots with timing; handle output path/seed parameters.  
  - **Dev Notes:** _Add observations, setup quirks, or reminders relevant to this step._
  
- [ ] 13) Performance tuning: cache allocations, preallocate regret tables, profiling; enable CFR+ if helpful: benchmark iterations/sec, adjust memory layout, optional CFR+ switch; document settings.  
  - **Dev Notes:** _Add observations, setup quirks, or reminders relevant to this step._
  
- [ ] 14) Robustness pass: input validation, error messages, config for iterations, seed, output path: add argument validation, graceful handling of missing ranges, safe defaults, and logging for failures.  
  - **Dev Notes:** _Add observations, setup quirks, or reminders relevant to this step._
  
- [ ] 15) Final integration tests and sample run producing a few JSONL lines: run end-to-end on small iteration count, validate JSONL schema, and capture sample output artifacts for reference.  
  - **Dev Notes:** _Add observations, setup quirks, or reminders relevant to this step._
  

## Next Steps to Start Coding
- Set up build system and empty module structure from the folder layout.
- Write unit tests for cards, ranges, and betting before solver work.
- Implement CFR toy game test harness to lock down solver core behavior.
