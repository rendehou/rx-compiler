#!/usr/bin/env bash
set -euo pipefail

task_dir="$(cd "$(dirname "$0")" && pwd)"
build_dir="$task_dir/build"

mkdir -p "$build_dir"
g++ -std=c++17 \
    "$task_dir/main.cpp" "$task_dir/parser.cpp" \
    -o "$build_dir/tiny_parser"

if ! "$build_dir/tiny_parser" > "$build_dir/actual.txt"; then
    echo "FAIL: parser execution failed"
    exit 1
fi

if ! diff -u "$task_dir/expected.txt" "$build_dir/actual.txt"; then
    echo "FAIL: actual output does not match expected.txt"
    exit 1
fi

echo "PASS: parser output matches expected.txt"
