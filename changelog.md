**This file is used to document changes from coding agents**
Logs must be in chronilogical order. Always append new logs to bottom of file.

Example Entry Format:
**12/13/2025** *9:39am CST*: Refactored {file name} from previous set up. Now the file {details of changes}
_____
**12/10/2025** *9:55pm CST*: Added RNG and timer utilities with deterministic tests; introduced util unit test target via CTest; prepared logging/RNG/timer utilities per Phase 2 plan.
**12/13/2025** *10:25am CST*: Implemented card/board utilities and board generator with blocker handling; added deck/board tests and wired `board_tests` into CTest; updated plan Dev Notes for Phase 3.
**12/13/2025** *1:30pm CST*: Added Linux-friendly build guidance and thread linkage; linked test/CLI targets against pthreads; introduced repo `.gitignore` to drop Windows build outputs and normalized test README for WSL use.
**12/13/2025** *2:05pm CST*: Moved pthread discovery to the top-level CMake so Threads::Threads resolves for test targets during Linux builds.
**12/13/2025** *2:20pm CST*: Updated AGENTS.md to mark WSL/Linux as primary environment, prefer Ninja for builds, and point preflop ranges to the current root `preflop-ranges/` directory.
**12/13/2025** *2:45pm CST*: Implemented phase 4 preflop range loader (`io/preflop_range.h`) with normalized combo weights and blocker-aware sampling; added `ranges_tests` covering parse/normalization and blocker exclusion; wired test target into CTest.
**12/13/2025** *3:05pm CST*: Added `scripts/test.sh` helper to configure (preferring Ninja), build, and run all unit tests via a single command.
**12/13/2025** *3:30pm CST*: Implemented phase 5 game state/actions/betting utilities (`core/game_state.h`) with legal action generation, min-raise enforcement, and stack/pot updates; added `actions_tests` for legality, min-raise, short-stack all-in handling, and raise application; all tests passing via CTest.
**12/13/2025** *3:50pm CST*: Hardened `scripts/test.sh` to reuse existing CMake generator (avoiding cache conflicts) and fixed syntax; verified it builds and runs all tests successfully.
**12/13/2025** *4:10pm CST*: Implemented phase 6 tree scaffolding with `core/tree.h` (node representation and deterministic info-set keys) and added `tree_tests` to validate key stability/owner distinction/terminal flag; full test suite passing via CTest.
**12/13/2025** *4:40pm CST*: Implemented phase 7 with a minimal CFR trainer for Kuhn poker (`solver/kuhn_cfr.h`) and added `cfr_tests` to validate convergence sanity (equilibrium structure and response behavior); verified full test suite passing via CTest.
**12/13/2025** *5:20pm CST*: Implemented phase 8 by adding `solver/nlhe_cfr.h` to run CFR over NLHE-style betting actions using the phase-5 legality/apply logic; extended `core/game_state.h` to track per-player committed chips and street completion for correct fold/showdown payoff math; added `nlhe_cfr_tests` covering payoff correctness on a tiny river tree; full test suite passing.
**12/13/2025** *1:30pm CST*: The project has been moved from a Windows filesystem to WSL (Ubuntu, Linux). The project directory was renamed from: "C++ Poker Solver CFR" to: "poker_solver_cfr"
**12/13/2025** *1:50pm CST*: Reinstalled cmake and ninja.
