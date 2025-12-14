# Repository Guidelines

## Project Structure & Module Organization
- `src/cli/`: argument parsing, progress display, orchestration.
- `src/core/`: cards, ranges, game state, actions, betting, board generation.
- `src/solver/`: CFR core, strategy storage, subtree expansion, rollouts.
- `src/io/`: JSONL writer, preflop range loader, config parsing.
- `src/util/`: RNG, logging, timers, metrics.
- `tests/`: mirrors `src/` for unit and integration tests.
- `preflop-ranges/`: provided preflop ranges (JSON) at repo root (align loader accordingly).
- `scripts/`: helper scripts (benchmarks, sample runs).

## Build, Test, and Development Commands
- Primary environment: WSL/Linux (Bash). Windows batch scripts in `scripts/` are legacy only.
- Configure (Debug recommended while developing): `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug` (omit `-G Ninja` if not installed).
- Build all: `cmake --build build -j$(nproc)` (or `cmake --build build` with Ninja handling jobs).
- Run unit/integration tests: `./scripts/test.sh` (or `ctest --test-dir build --output-on-failure`).
- Sample run (writes/validates JSONL): `./scripts/sample_run.sh`.
- Benchmark + quality report: `./scripts/bench.sh`.
- Sample run (after build): `./build/bin/solver --number_of_situations 10 --output out.jsonl --seed 42 --iterations 2000 --branch_threshold 0.20`.
  - JSONL resume/dedupe: the CLI computes `metadata.spot_id` and skips any spots already present in the existing output file.
  - Durability: the CLI flushes each JSONL line so completed spots survive early exit (SIGINT/SIGTERM).

## Coding Style & Naming Conventions
- C++17/20, 2- or 4-space indentation (no tabs); be consistent with nearby code.
- Filenames: lowercase with underscores (e.g., `game_state.h`).
- Types: `CamelCase`; functions/variables: `snake_case`; constants: `kPascalCase`.
- Prefer `std::unique_ptr`/`std::shared_ptr` over raw owning pointers; pass by const ref where possible.
- Keep headers minimal; avoid unnecessary includes; use forward declarations in headers.
- `changelog.md` needs to be updated after every coding session. Document what changes you made and code you added.

## Testing Guidelines
- Framework: use standard CTest harness; keep tests small and deterministic (seeded RNG).
- Naming: mirror target (e.g., `cards_tests.cpp`, `cfr_tests.cpp`).
- Coverage: exercise edge cases (all-in caps, action legality, collision checks) and a toy CFR game (e.g., Kuhn) for convergence sanity.
- Run `ctest` before opening PRs; add regression tests for bug fixes.

## Commit & Pull Request Guidelines
- Commits: clear, imperative subject (e.g., “Add board generator validation”); group related changes.
- PRs: include summary, test evidence (commands and outcomes), linked issues/task IDs, and notes on performance impact if relevant.
- Screenshots not required (CLI), but include sample JSONL line when changing output shape.

## Architecture Overview (Quick)
- Pipeline: scenario generation → CFR root solve → sub-branch solves (≥20% hero actions) with villain optimal → JSONL append.
- Outputs: one JSON object per line with metadata, ranges, root strategy, solved branches, board, metrics.

## Security & Configuration Tips
- Keep RNG seeds explicit for reproducibility.
- Validate inputs: range files present, output path writable, iteration counts sane.
- Avoid unchecked dynamic allocations in hot paths; preallocate tables where possible.
