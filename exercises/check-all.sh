#!/usr/bin/env bash
set -euo pipefail

examples_dir="$(cd "$(dirname "$0")" && pwd)"
for check_script in "$examples_dir"/[0-9][0-9]-*/check.sh; do
    "$check_script"
done
