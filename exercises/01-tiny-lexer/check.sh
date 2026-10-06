#!/usr/bin/env bash
set -euo pipefail

task_dir="$(cd "$(dirname "$0")" && pwd)"
build_dir="$task_dir/build"

mkdir -p "$build_dir"
g++ -std=c++17 \
    "$task_dir/main.cpp" "$task_dir/lexer.cpp" \
    -o "$build_dir/tiny_lexer"

"$build_dir/tiny_lexer" > "$build_dir/actual.txt"

if ! diff -u "$task_dir/expected.txt" "$build_dir/actual.txt"; then
    echo "FAIL: actual output does not match expected.txt"
    exit 1
fi

echo "PASS: lexer output matches expected.txt"
