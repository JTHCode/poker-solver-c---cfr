#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
BUILD_TYPE="Debug"
SITUATIONS=5
ITERATIONS=200
BRANCH_THRESHOLD="0.20"
SEED=42
OUTPUT_PATH="out_sample.jsonl"
NO_BUILD=0

usage() {
  cat <<EOF
Usage: $0 [options]

Builds (optional) and runs a small end-to-end solve, writing a JSONL output file and validating it.

Options:
  --build-dir <dir>         Build directory (default: ${BUILD_DIR})
  --build-type <type>       CMake build type (default: ${BUILD_TYPE})
  --situations <n>          Number of situations (default: ${SITUATIONS})
  --iterations <n>          CFR iterations per solve (default: ${ITERATIONS})
  --branch-threshold <t>    Root branch threshold in [0,1] (default: ${BRANCH_THRESHOLD})
  --seed <seed>             RNG seed (default: ${SEED})
  --output <path>           JSONL output path (default: ${OUTPUT_PATH})
  --no-build                Skip CMake configure/build
  -h, --help                Show help

Examples:
  $0
  $0 --build-dir build_rel --build-type Release --situations 20 --iterations 500 --output out.jsonl
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
    --no-build) NO_BUILD=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown arg: $1" >&2; usage; exit 2 ;;
  esac
done

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
  exit 1
fi

echo "Running: ${SOLVER} --number_of_situations ${SITUATIONS} --iterations ${ITERATIONS} --branch_threshold ${BRANCH_THRESHOLD} --seed ${SEED} --output ${OUTPUT_PATH}"
"${SOLVER}" \
  --number_of_situations "${SITUATIONS}" \
  --iterations "${ITERATIONS}" \
  --branch_threshold "${BRANCH_THRESHOLD}" \
  --seed "${SEED}" \
  --progress_every 1 \
  --output "${OUTPUT_PATH}"

if command -v python3 >/dev/null 2>&1; then
  python3 - <<'PY' "${OUTPUT_PATH}" "${SITUATIONS}"
import json
import sys

path = sys.argv[1]
expected = int(sys.argv[2])

required_top = ["metadata", "ranges", "root_strategy", "solved_branches", "board", "metrics"]
seen = set()
lines = 0

with open(path, "r", encoding="utf-8") as f:
  for raw in f:
    raw = raw.strip()
    if not raw:
      continue
    lines += 1
    obj = json.loads(raw)
    for k in required_top:
      if k not in obj:
        raise SystemExit(f"Missing top-level key: {k}")
    sid = obj["metadata"].get("spot_id")
    if not isinstance(sid, str) or not sid:
      raise SystemExit("Missing metadata.spot_id")
    if sid in seen:
      raise SystemExit(f"Duplicate spot_id: {sid}")
    seen.add(sid)
    acts = obj["root_strategy"]["actions"]
    probs = obj["root_strategy"]["probs"]
    if len(acts) != len(probs) or len(acts) == 0:
      raise SystemExit("root_strategy actions/probs invalid")
    s = sum(float(p) for p in probs)
    if abs(s - 1.0) > 1e-6:
      raise SystemExit(f"root_strategy probs do not sum to 1 (sum={s})")

if lines != expected:
  raise SystemExit(f"Expected {expected} JSONL lines, got {lines}")

print("Sample run validation: OK")
PY
else
  echo "NOTE: python3 not found; skipped JSONL validation." >&2
fi

echo
echo "First JSONL line (truncated):"
head -n 1 "${OUTPUT_PATH}" | cut -c 1-240

