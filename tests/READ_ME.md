**WSL/Linux build & test (recommended):**

```bash
./scripts/test.sh
```

**Benchmarking (situations/minute):**

Runs the solver for a fixed number of situations and prints throughput plus basic JSONL stats (avg/p50/p90/p99 solve time, branches). Uses a temporary JSONL file by default.

```bash
./scripts/bench.sh
./scripts/bench.sh --situations 1000 --iterations 500
./scripts/bench.sh --build-dir build_rel --build-type Release --situations 200 --iterations 2000
./scripts/bench.sh --keep-output                      # keeps the temp JSONL and prints its path
./scripts/bench.sh --output /tmp/run.jsonl --keep-output
```

**Windows (legacy):**

```powershell
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```
