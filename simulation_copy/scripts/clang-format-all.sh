#!/usr/bin/env bash

# safe https://sipb.mit.edu/doc/safe-shell/
set -euf -o pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
SRC_DIR="$(dirname "$SCRIPT_DIR")"

# Format each
for dir_name in "simulation_cli" "simulation_lib" "simulation_tests" "simulation_visualization"
do
    echo "Formatting $dir_name"
    "$SCRIPT_DIR/clang-format.sh" "$SRC_DIR/$dir_name"
done
