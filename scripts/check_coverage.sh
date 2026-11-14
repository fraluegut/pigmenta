#!/usr/bin/env bash
# Configures a throwaway build with tests + coverage instrumentation, runs
# every suite, and fails if line coverage on the modules we hold to 100%
# drops below threshold. Used by the pre-commit hook and CI.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT/build-coverage"
THRESHOLD=100

cmake -B "$BUILD_DIR" -S "$ROOT" \
    -DPIGMENTA_BUILD_TESTS=ON -DPIGMENTA_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug \
    >/dev/null

cmake --build "$BUILD_DIR" -j"$(nproc)" >/dev/null

"$BUILD_DIR/core/pigmenta_core_tests"
"$BUILD_DIR/cli/pigmenta_cli_tests"
if [ -n "${DISPLAY:-}" ] || command -v xvfb-run >/dev/null; then
    xvfb-run -a "$BUILD_DIR/desktop/pigmenta_desktop_tests"
else
    "$BUILD_DIR/desktop/pigmenta_desktop_tests"
fi

# core/ is pure, deterministic math — held to a strict 100% line coverage
# gate. cli/ and desktop/ only include their testable (non-GUI, non-main)
# logic here; see docs/TESTING.md for what's intentionally excluded.
pushd "$BUILD_DIR" >/dev/null
SUMMARY=$(gcovr --root "$ROOT" \
    --filter "$ROOT/core/src/" --filter "$ROOT/core/include/" \
    --exclude '.*_test\.cpp' -s)
popd >/dev/null

echo "$SUMMARY"
LINE_PCT=$(echo "$SUMMARY" | grep '^lines:' | grep -oE '[0-9]+\.[0-9]+' | head -1)

if (( $(echo "$LINE_PCT < $THRESHOLD" | bc -l) )); then
    echo "FAIL: core/ line coverage ${LINE_PCT}% is below the ${THRESHOLD}% gate." >&2
    exit 1
fi

echo "OK: core/ is at ${LINE_PCT}% line coverage."
