# Testing

Unit tests live next to the file they test (`foo.cpp` → `foo_test.cpp`),
using [Catch2](https://github.com/catchorg/Catch2) v3. They're opt-in via a
CMake flag so nothing outside this repo's own top-level build — the Android
NDK build, a standalone `cli/`-only build — ever pulls in a test framework.

## Running the tests

```sh
cmake -B build -S . -DPIGMENTA_BUILD_TESTS=ON
cmake --build build -j
./build/core/pigmenta_core_tests
./build/cli/pigmenta_cli_tests
./build/desktop/pigmenta_desktop_tests   # needs a display, or xvfb-run
```

The first configure fetches Catch2 via `FetchContent` (one-time, needs
network).

## Coverage

```sh
./scripts/check_coverage.sh
```

Configures a throwaway `build-coverage/` with `-DPIGMENTA_COVERAGE=ON`,
builds, runs every suite, and reports `core/`'s line coverage via `gcovr`,
failing if it drops below 100%.

## What's covered, and what isn't — and why

**`core/`** — the native image-processing engine — is held to a strict
**100% line and function coverage** gate. It's pure, deterministic C++ with
no I/O and no platform dependency, so there's no excuse not to test every
line. Branch coverage sits at ~85%; every branch gcovr reports as "missing"
was checked by hand and is one of:

- A compiler-inserted branch around an `std::vector`/STL allocation or
  algorithm call (`std::min_element`, `partial_sum`, …) — not application
  logic, not something a test can meaningfully exercise without forcing
  `bad_alloc`.
- The Jacobi eigendecomposition's 50-sweep safety cap in
  `matrix3x3.h` — real symmetric matrices converge in a handful of sweeps;
  forcing non-convergence to hit the loop-exhaustion branch would be
  adversarial, not representative.

Chasing literal 100% branch coverage here would mean writing tests against
compiler artifacts, not behaviour — so we don't.

**`cli/`** — only the pure argument-parsing helpers in `args.h`/`args.cpp`
are unit tested (`args_test.cpp`). `main.cpp` itself (argv parsing loop,
file I/O, process exit codes) is exercised by hand/smoke-tested, not under
the coverage gate — testing it would mean either shelling out to the built
binary (an integration test, not a unit test) or refactoring the entire
CLI around dependency injection for marginal benefit.

**`desktop/`** — `Translator` (every key resolves in both languages,
language switching) and `Engine` (filter specs, `applyFilter` on real
`QImage`s) are unit tested. The Qt widgets themselves (`MainWindow`,
`CompareView`, dialogs, drag-and-drop) are **not** unit tested — GUI event
and paint-event testing has a poor cost/benefit ratio for a small team, and
is instead verified by hand before each change ships (see the repo's commit
history for what was manually checked).

**`android/`** (Kotlin) — **not covered by this pass.** It needs its own
toolchain (JUnit/Robolectric, Android SDK), which is a separate, larger
effort from the C++ side above. Tracked as follow-up work, not silently
skipped.

## Pre-commit

```sh
pip install pre-commit
pre-commit install
```

Runs on every commit: generic hygiene (trailing whitespace, EOF newlines,
merge conflict markers, large-file guard), `clang-format` on `core/`,
`cli/`, `desktop/`, `bindings/` (excluding the hand-aligned numeric matrices
in `core/src/color_spaces.cpp`, `decorrelation.cpp`, `filters.cpp` and
`core/include/pigmenta/matrix3x3.h`, where clang-format would collapse
deliberate column alignment of coefficients into unreadable single-spaced
math), and the `core/` coverage gate above.

Not run by pre-commit: Python tooling (`ruff`/`black`) — there is no Python
in this repo, so they'd be no-ops; Kotlin linting (`ktlint`/`detekt`) for
`android/` — follow-up work, same as its test coverage.
