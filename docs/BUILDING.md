# Building

## Core + CLI (any desktop OS, no Android SDK needed)

```sh
cmake -B build -S .
cmake --build build --config Release
./build/cli/pigmenta-cli --help
```

Requires CMake >= 3.22 and a C++17 compiler. No other dependencies.

## Android app

```sh
cd android
./gradlew assembleDebug
```

Requires Android SDK + NDK (configured via `local.properties` / Android
Studio). The app's CMake build compiles `core/` and `bindings/jni/` for you —
no separate step needed.

## Desktop GUI

See [desktop/README.md](../desktop/README.md). Requires Qt6 (`qt6-base-dev`
on Debian/Ubuntu). Built automatically by the root `cmake -B build -S .` when
Qt6 is detected; skipped otherwise.

## Tests, coverage, pre-commit

See [docs/TESTING.md](TESTING.md).
