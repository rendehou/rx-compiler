#!/usr/bin/env bash
set -euo pipefail

task_dir="$(cd "$(dirname "$0")" && pwd)"
build_dir="$task_dir/build"

cmake -S "$task_dir" -B "$build_dir" >/dev/null
cmake --build "$build_dir" --parallel 4 >/dev/null
actual_file="$build_dir/antlr_ast_actual.txt"
"$build_dir/rx_antlr_ast_example" > "$actual_file"

if ! diff -u "$task_dir/expected.txt" "$actual_file"; then
    echo "FAIL: actual output does not match expected.txt"
    exit 1
fi

echo "PASS: ANTLR bridge output matches expected.txt"
