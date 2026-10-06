#!/usr/bin/env bash
set -euo pipefail
task_dir="$(cd "$(dirname "$0")" && pwd)"
build_dir="$task_dir/build"
mkdir -p "$build_dir"
g++ -std=c++17 "$task_dir/main.cpp" -o "$build_dir/control_flow"
"$build_dir/control_flow" > "$build_dir/actual.txt"
diff -u "$task_dir/expected.txt" "$build_dir/actual.txt"
echo "PASS: control-flow output matches expected.txt"
