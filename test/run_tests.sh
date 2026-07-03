#!/bin/sh
# Build and run the determinism regression test against the core.
# Usage: test/run_tests.sh [path-to-core.so]
# Builds the core first if no path is given and none exists.
set -e
cd "$(dirname "$0")/.."

CORE="${1:-./stella2014_libretro.so}"
[ -f "$CORE" ] || make -j"$(nproc 2>/dev/null || echo 2)"

cc -O2 -o test/determinism_harness test/determinism_harness.c \
   -I libretro-common/include -ldl

./test/determinism_harness "$CORE"

if command -v valgrind >/dev/null 2>&1; then
    echo "running under valgrind..."
    valgrind -q --leak-check=full --error-exitcode=42 \
        ./test/determinism_harness "$CORE" >/dev/null
    echo "valgrind: clean"
else
    echo "valgrind not found: skipping leak check"
fi
echo "all tests passed"
