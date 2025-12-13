#!/usr/bin/env bash
set -euo pipefail

# Simple test runner: configure (prefers Ninja if available), build, and run CTest.

BUILD_TYPE="${1:-Debug}"

GEN_ARGS=()
if command -v ninja >/dev/null 2>&1; then
  GEN_ARGS=(-G Ninja)
fi

cmake -S . -B build "${GEN_ARGS[@]}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
cmake --build build
ctest --test-dir build --output-on-failure
