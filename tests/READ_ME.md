**WSL/Linux build & test (recommended):**

```bash
./scripts/test.sh
```

Notes:
- The unit test suite includes an end-to-end CLI smoke test (`cli_smoke_tests`) that runs the solver for a few spots and validates JSONL output shape + `metadata.spot_id` uniqueness.

**Sample run (writes JSONL + validates):**

```bash
./scripts/sample_run.sh
./scripts/sample_run.sh --build-dir build_rel --build-type Release --situations 20 --iterations 500 --output out.jsonl
```

**Benchmarking (situations/minute):**

Runs the solver for a fixed number of situations and prints throughput plus basic JSONL stats (avg/p50/p90/p99 solve time, branches). Uses a temporary JSONL file by default.

**Benchmark contract (recommended defaults):**
- Use Release for benchmarks (`--build-dir build_rel --build-type Release`).
- Defaults: `--situations 200 --iterations 2000 --branch-threshold 0.20 --seed 42`.

```bash
./scripts/bench.sh
./scripts/bench.sh --situations 1000 --iterations 500
./scripts/bench.sh --build-dir build_rel --build-type Release --situations 200 --iterations 2000
./scripts/bench.sh --keep-output                      # keeps the temp JSONL and prints its path
./scripts/bench.sh --output /tmp/run.jsonl --keep-output
./scripts/bench.sh --no-quality                      # omit the quality_report run
```

**Quality metrics (exploitability):**

Build and run:

```bash
cmake --build build -j$(nproc)
./build/tests/quality_report --kuhn_iterations 20000 --nlhe_iterations 400 --seed 12345
```

Notes:
- Prints Kuhn exploitability and a small river-only NLHE exploitability estimate (both as `0.5 * (BR0 - BR1)`).

**Windows (legacy / not maintained):**

```powershell
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```
