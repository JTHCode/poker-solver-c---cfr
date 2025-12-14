# Poker Solver (CFR Scaffold)

This repository contains a Linux/WSL-first C++17 poker solver scaffold built around vanilla CFR. It generates random postflop heads-up NLHE situations (from provided preflop ranges), solves a small abstracted betting game, and appends one JSON object per situation to a JSONL output file.

Current scope:
- Correct hold’em showdown evaluation + terminal payoff handling (including all-in runouts).
- A fixed betting abstraction (see `details.md`) with legality tests.
- Repeatable benchmarks + quality metrics (see `benchmarks.md` and `tests/READ_ME.md`).
- Output resume/dedupe via deterministic `metadata.spot_id` when appending to an existing JSONL file.

## Requirements

- Linux / WSL (primary environment)
- CMake ≥ 3.16
- A C++17 compiler (e.g., `g++`)
- Optional but recommended: Ninja (`ninja-build`)
- Optional: `python3` (used by `scripts/bench.sh` / `scripts/sample_run.sh` for richer JSONL validation)

Ubuntu/WSL install:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build python3
```

## Quickstart

Run unit + integration tests (includes an end-to-end CLI smoke test):

```bash
./scripts/test.sh
```

Run a small sample solve (writes/validates JSONL and prints a truncated sample line):

```bash
./scripts/sample_run.sh
```

Run a benchmark (throughput + JSONL stats + optional quality report):

```bash
./scripts/bench.sh
```

## Build

Debug (development):

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Release (benchmarking):

```bash
cmake -S . -B build_rel -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build_rel
```

The solver binary is written to `build/bin/solver` (or `build_rel/bin/solver`).

## Run The Solver

```bash
./build/bin/solver \
  --number_of_situations 10 \
  --output out.jsonl \
  --seed 42 \
  --iterations 2000 \
  --branch_threshold 0.20
```

Key flags:
- `--number_of_situations`: required; number of unique spots to write.
- `--output`: JSONL output path (append-only).
- `--seed`: RNG seed for reproducibility.
- `--iterations`: CFR iterations per solve.
- `--branch_threshold`: root action cutoff for branch solving.

Startup validation:
- The CLI validates that the output directory exists and is writable.
- The CLI validates that the required `preflop-ranges/` files exist and parse correctly.

Append/resume behavior:
- The CLI scans the existing output file (if any) and skips any scenario whose `metadata.spot_id` is already present.
- Each JSONL line is flushed immediately so completed spots survive early exit (SIGINT/SIGTERM).

## Output (JSONL)

Each output line is a single JSON object with these top-level keys:
- `metadata`: identifiers (including `spot_id`), positions, seed, preflop line
- `ranges`: sampled hole cards
- `board`: flop/turn/river
- `root_strategy`: action labels + probabilities at the root
- `solved_branches`: per-branch solves for frequent hero root actions
- `metrics`: solve time and traversal stats

Schema and basic correctness are validated by:
- `tests/jsonl_writer_tests.cpp`
- `tests/cli_smoke_tests.cpp`

## Docs

- `plan.md`: original build plan and phased development checklist.
- `upgrade-plan.md`: pre-CFR+ readiness plan and final gates verification.
- `details.md`: key modeling/abstraction decisions.
- `tests/READ_ME.md`: build/test/benchmark commands and benchmark “contract”.
- `benchmarks.md`: baseline throughput + quality numbers for comparison work.
- `changelog.md`: running log of coding-agent changes.

## Repo Layout

- `src/cli/`: argument parsing + solve loop + progress logging
- `src/core/`: cards, board generation, game state, legality, tree/keying
- `src/solver/`: CFR, tree building, subtree expansion, metrics
- `src/io/`: JSONL writer, preflop range loader, JSON helpers
- `src/util/`: RNG, logging, timers, hashing
- `tests/`: CTest executables (unit + smoke integration)
- `preflop-ranges/`: provided preflop ranges (repo-root)
- `scripts/`: `test.sh`, `bench.sh`, `sample_run.sh`

