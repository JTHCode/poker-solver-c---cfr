#!/usr/bin/env bash
set -euo pipefail

# Simple test runner: configure, build, run CTest, and (optionally) check JSONL for duplicate spot_id.

BUILD_TYPE="${1:-Debug}"
JSONL_PATH="${2:-}"

GEN_ARGS=()
if [[ -f build/CMakeCache.txt ]]; then
  EXISTING_GEN=$(grep -E '^CMAKE_GENERATOR:INTERNAL=' build/CMakeCache.txt | cut -d= -f2-)
  echo "Reusing existing generator: ${EXISTING_GEN}"
else
  if command -v ninja >/dev/null 2>&1; then
    GEN_ARGS=(-G Ninja)
  else
    GEN_ARGS=(-G "Unix Makefiles")
  fi
fi

cmake -S . -B build "${GEN_ARGS[@]}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
cmake --build build
ctest --test-dir build --output-on-failure

if [[ -z "${JSONL_PATH}" && -f "out.jsonl" ]]; then
  JSONL_PATH="out.jsonl"
fi

if [[ -n "${JSONL_PATH}" && -f "${JSONL_PATH}" ]]; then
  if command -v python3 >/dev/null 2>&1; then
    python3 - <<'PY' "${JSONL_PATH}"
import json, sys
path = sys.argv[1]
seen = set()
dupes = 0
missing = 0
bad = 0
with open(path, "r", encoding="utf-8") as f:
    for line_no, line in enumerate(f, 1):
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
            continue
        if sid in seen:
            dupes += 1
        else:
            seen.add(sid)

if bad:
    print(f"[jsonl] WARNING: {bad} malformed JSON line(s) in {path}; skipping those lines.")
if missing:
    print(f"[jsonl] WARNING: {missing} line(s) missing metadata.spot_id in {path}; cannot fully dedupe-check.")
if dupes:
    print(f"[jsonl] ERROR: found {dupes} duplicate spot_id(s) in {path}.")
    sys.exit(1)
print(f"[jsonl] OK: no duplicate spot_id found in {path} (checked {len(seen)} spot_id).")
PY
  else
    echo "[jsonl] Skipping duplicate check: python3 not found."
  fi
fi
