#!/usr/bin/env bash

# safe https://sipb.mit.edu/doc/safe-shell/
# set -eu -o pipefail

function usage()
{
    echo "Usage: $0 <directory|file>|--help"
}

# Try different clang-format commands...
if type "clang-tidy" > /dev/null 2>&1; then
    CLANG_TIDY="clang-tidy"
else
    CLANG_TIDY=""
fi

if [ -z "$CLANG_TIDY" ]; then
    echo "No clang-tidy command can be found"
    exit 1
fi
echo "Using: $("$CLANG_TIDY" --version)"

# no arguments
if [ "$#" == "0" ]; then
    usage
    exit 1
fi

path="$1"
# display help
if [ "$path" = "--help" ]; then
    usage
    exit 0
fi

# TODO(vampy):
# https://gist.github.com/flamewing/a5016645064afa086ac8afd98ea7f627
# https://clang.llvm.org/extra/doxygen/run-clang-tidy_8py_source.html
# https://github.com/KratosMultiphysics/Kratos/wiki/How-to-use-Clang-Tidy-to-automatically-correct-code

# path does not exist
echo "Tidying..."
if  [ -d "$path" ]; then
    "$CLANG_TIDY" -header-filter='.*' "$path/*"
elif [ -f "$path" ]; then
    "$CLANG_TIDY" -header-filter='.*' "$path"
fi
