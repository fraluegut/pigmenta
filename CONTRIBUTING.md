# Contributing

Thanks for considering a contribution to Pigmenta.

## Where things live

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) first — it explains the
`core/` / `cli/` / `android/` / `desktop/` split and why algorithm changes
only ever happen in one place.

## Workflow

1. Fork, branch off `master`.
2. `pip install pre-commit && pre-commit install` once, so formatting and
   the `core/` coverage gate run on every commit (see
   [docs/TESTING.md](docs/TESTING.md)).
3. Build the piece you're changing (see [docs/BUILDING.md](docs/BUILDING.md))
   and confirm it still builds before opening a PR.
4. If you touch `core/`, add/update its tests (`core/src/*_test.cpp`) —
   it's held to 100% line coverage — and check both the CLI and the Android
   app still build against the new signatures (`bindings/jni/` and the
   Kotlin `external fun` declarations must match).
5. Open a pull request describing the change and why.

## Code style

- C++: match the existing style in `core/` (17, `snake_case` free functions,
  raw pixel buffers, no exceptions across the engine boundary).
- Kotlin: standard Android/Kotlin conventions already used in `android/app`.

## Reporting bugs / proposing filters

Open an issue. For a new filter, a before/after image pair and the math/paper
it's based on (DStretch-style decorrelation, CLAHE, etc.) makes review much
faster.
