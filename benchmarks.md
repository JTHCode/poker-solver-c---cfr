# Benchmarks (Baseline)

This file records baseline performance and quality numbers for the current solver scaffold.

Notes:
- Benchmarks should be run in Release (`./scripts/bench.sh` defaults to `build_rel` + `Release`).
- Throughput is reported as situations/min.
- Quality is reported as exploitability from `build/tests/quality_report` (Kuhn + a small river-only NLHE abstraction).
- Reproduce: `./scripts/bench.sh` (or `./scripts/bench.sh --situations 200 --iterations 2000 --seed 42 --branch-threshold 0.20`).

## Environment

- Date: 2025-12-14
- OS: WSL/Linux (`Linux MSI 6.6.87.2-microsoft-standard-WSL2`)
- Compiler: g++ (from CMake configure output)
- CPU: Intel(R) Core(TM) i7-8565U CPU @ 1.80GHz (4 cores / 8 threads)

## Results

### Standard config (seed=42, branch_threshold=0.20, situations=200)

- Iterations=500:
  - situations/min: 872.6
  - solve_time_ms_avg: 68.62 (p50=60.86, p90=130.15, p99=174.77)
  - nodes/sec: 9181947.2 (decision_nodes/sec: 3078901.8)
  - branches_avg: 1.510
  - quality: Kuhn exploitability=0.00160785; NLHE river-only exploitability=0.0734712
- Iterations=2000:
  - situations/min: 277.1
  - solve_time_ms_avg: 216.40 (p50=218.27, p90=360.98, p99=479.87)
  - nodes/sec: 11662988.4 (decision_nodes/sec: 3910847.6)
  - branches_avg: 1.510
  - quality: Kuhn exploitability=0.00160785; NLHE river-only exploitability=0.0183678
- Iterations=10000:
  - situations/min: 48.3
  - solve_time_ms_avg: 1241.14 (p50=1141.59, p90=2168.19, p99=4720.36)
  - nodes/sec: 10172006.7 (decision_nodes/sec: 3410889.8)
  - branches_avg: 1.510
  - quality: Kuhn exploitability=0.00160785; NLHE river-only exploitability=0.00367356
