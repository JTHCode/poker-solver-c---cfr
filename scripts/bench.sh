#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build_rel"
BUILD_TYPE="Release"
SITUATIONS=200
ITERATIONS=2000
BRANCH_THRESHOLD="0.20"
SEED=42
KEEP_OUTPUT=0
NO_BUILD=0
OUTPUT_PATH=""

usage() {
  cat <<EOF
Usage: $0 [options]

Runs the solver for N situations and prints throughput + basic JSONL stats.

Options:
  --build-dir <dir>         Build directory (default: ${BUILD_DIR})
  --build-type <type>       CMake build type (default: ${BUILD_TYPE})
  --situations <n>          Number of situations to generate/solve (default: ${SITUATIONS})
  --iterations <n>          CFR iterations per solve (default: ${ITERATIONS})
  --branch-threshold <t>    Root branch threshold in [0,1] (default: ${BRANCH_THRESHOLD})
  --seed <seed>             RNG seed (default: ${SEED})
  --output <path>           JSONL output path (default: temp file)
  --keep-output             Keep the output JSONL file (default: off)
  --no-build                Skip CMake configure/build (default: off)
  -h, --help                Show help

Examples:
  $0
  $0 --situations 1000 --iterations 500
  $0 --build-type Debug --build-dir build --situations 50 --iterations 2000
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir) BUILD_DIR="${2:?}"; shift 2 ;;
    --build-type) BUILD_TYPE="${2:?}"; shift 2 ;;
    --situations) SITUATIONS="${2:?}"; shift 2 ;;
    --iterations) ITERATIONS="${2:?}"; shift 2 ;;
    --branch-threshold) BRANCH_THRESHOLD="${2:?}"; shift 2 ;;
    --seed) SEED="${2:?}"; shift 2 ;;
    --output) OUTPUT_PATH="${2:?}"; shift 2 ;;
    --keep-output) KEEP_OUTPUT=1; shift ;;
    --no-build) NO_BUILD=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown arg: $1" >&2; usage; exit 2 ;;
  esac
done

if [[ -z "${OUTPUT_PATH}" ]]; then
  OUTPUT_PATH="$(mktemp -t poker_solver_bench_XXXXXX.jsonl)"
fi

cleanup() {
  if [[ "${KEEP_OUTPUT}" -eq 0 ]]; then
    rm -f "${OUTPUT_PATH}" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

if [[ "${NO_BUILD}" -eq 0 ]]; then
  GEN_ARGS=()
  if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
    if command -v ninja >/dev/null 2>&1; then
      GEN_ARGS=(-G Ninja)
    else
      GEN_ARGS=(-G "Unix Makefiles")
    fi
  fi
  cmake -S . -B "${BUILD_DIR}" "${GEN_ARGS[@]}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
  cmake --build "${BUILD_DIR}"
fi

SOLVER="${BUILD_DIR}/bin/solver"
if [[ ! -x "${SOLVER}" ]]; then
  echo "ERROR: solver binary not found/executable at: ${SOLVER}" >&2
  echo "Try: $0 --build-dir <dir> --build-type <type>" >&2
  exit 1
fi

CMD=(
  "${SOLVER}"
  --number_of_situations "${SITUATIONS}"
  --output "${OUTPUT_PATH}"
  --seed "${SEED}"
  --iterations "${ITERATIONS}"
  --branch_threshold "${BRANCH_THRESHOLD}"
  --progress_every 1000000000
)

start_ns="$(date +%s%N)"
"${CMD[@]}" >/dev/null
end_ns="$(date +%s%N)"

if command -v python3 >/dev/null 2>&1; then
  python3 - <<'PY' "${OUTPUT_PATH}" "${SITUATIONS}" "${ITERATIONS}" "${BRANCH_THRESHOLD}" "${BUILD_DIR}" "${BUILD_TYPE}" "${SEED}" "${SOLVER}" "${start_ns}" "${end_ns}"
import json
import math
import os
import sys

path, situations, iterations, threshold, build_dir, build_type, seed, solver_path, start_ns, end_ns = sys.argv[1:]
situations = int(situations)
iterations = int(iterations)
threshold = float(threshold)
seed = int(seed)
start_ns = int(start_ns)
end_ns = int(end_ns)

elapsed_s = (end_ns - start_ns) / 1e9
spm = (situations / elapsed_s) * 60.0 if elapsed_s > 0 else float("inf")

count = 0
solve_ms = []
branches = []
dupes = 0
seen = set()
bad = 0
missing = 0
nodes_visited = 0
decision_nodes = 0
terminal_evals = 0
legal_actions_total = 0
chance_samples = 0

with open(path, "r", encoding="utf-8") as f:
  for line in f:
    line = line.strip()
    if not line:
      continue
    try:
      obj = json.loads(line)
    except Exception:
      bad += 1
      continue
    meta = obj.get("metadata", {})
    sid = meta.get("spot_id")
    if not isinstance(sid, str) or not sid:
      missing += 1
    else:
      if sid in seen:
        dupes += 1
      else:
        seen.add(sid)
    m = obj.get("metrics", {})
    ms = m.get("solve_time_ms")
    if isinstance(ms, (int, float)):
      solve_ms.append(float(ms))
    nodes_visited += int(m.get("nodes_visited") or 0)
    decision_nodes += int(m.get("decision_nodes") or 0)
    terminal_evals += int(m.get("terminal_evals") or 0)
    legal_actions_total += int(m.get("legal_actions_total") or 0)
    chance_samples += int(m.get("chance_samples") or 0)
    b = obj.get("solved_branches", [])
    if isinstance(b, list):
      branches.append(len(b))
    count += 1

def pct(values, p):
  if not values:
    return float("nan")
  vals = sorted(values)
  idx = int(math.floor((p/100.0) * (len(vals)-1)))
  return vals[idx]

def avg(values):
  return sum(values)/len(values) if values else float("nan")

print("**Benchmark**")
print(f"- build: {build_dir} ({build_type})")
print(f"- solver: {solver_path}")
print(f"- args: --number_of_situations {situations} --iterations {iterations} --branch_threshold {threshold} --seed {seed}")
print(f"- output: {path}")
print(f"- elapsed_s: {elapsed_s:.3f}")
print(f"- situations_per_min: {spm:.1f}")
print(f"- lines_written: {count}")
if solve_ms:
  print(f"- solve_time_ms_avg: {avg(solve_ms):.2f}")
  print(f"- solve_time_ms_p50: {pct(solve_ms, 50):.2f}")
  print(f"- solve_time_ms_p90: {pct(solve_ms, 90):.2f}")
  print(f"- solve_time_ms_p99: {pct(solve_ms, 99):.2f}")
if branches:
  print(f"- branches_avg: {avg(branches):.3f}")
  print(f"- branches_min: {min(branches)}")
  print(f"- branches_max: {max(branches)}")
if elapsed_s > 0 and nodes_visited > 0:
  print(f"- nodes_per_sec: {nodes_visited/elapsed_s:.1f}")
  print(f"- decision_nodes_per_sec: {decision_nodes/elapsed_s:.1f}")
else:
  print(f"- nodes_per_sec: n/a")
  print(f"- decision_nodes_per_sec: n/a")
print(f"- avg_legal_actions_per_decision: {(legal_actions_total/decision_nodes):.3f}" if decision_nodes else "- avg_legal_actions_per_decision: n/a")
print(f"- chance_samples_total: {chance_samples}")
if bad:
  print(f"- WARNING: malformed_json_lines: {bad}")
if missing:
  print(f"- WARNING: missing_spot_id_lines: {missing}")
if dupes:
  print(f"- WARNING: duplicate_spot_id_lines: {dupes}")
PY
else
  elapsed_ns=$(( end_ns - start_ns ))
  echo "Benchmark:"
  echo "- build: ${BUILD_DIR} (${BUILD_TYPE})"
  echo "- elapsed_s: (python3 required for float math)"
  echo "- lines_written: (python3 required for JSONL stats)"
fi

if [[ "${KEEP_OUTPUT}" -eq 1 ]]; then
  echo "Kept output JSONL: ${OUTPUT_PATH}"
fi
