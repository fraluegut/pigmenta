# Pigmenta

Rock-art / petroglyph image enhancement: decorrelation stretch, CLAHE,
relief shading and other DStretch-style filters, built on a single shared
C++ engine and usable from Android, the command line, or a desktop GUI.

## Repository layout

| Path         | What it is                                                    |
|--------------|----------------------------------------------------------------|
| [core/](core/)       | Shared C++ image-processing engine. Platform-independent. |
| [cli/](cli/)         | Command-line tool — batch-process images on Linux/macOS/Windows. |
| [android/](android/) | Android app (camera capture, filters, field report, gallery). |
| [desktop/](desktop/) | Qt6 desktop GUI — drag & drop, live filter params, before/after compare. |
| [bindings/jni/](bindings/jni/) | JNI bridge between `core/` and the Android app. |
| [docs/](docs/)       | Architecture and build documentation. |

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for why this is one repo and
[docs/BUILDING.md](docs/BUILDING.md) for build instructions for each piece.

## Quick start

### Desktop GUI

Needs Qt6 (`qt6-base-dev` on Debian/Ubuntu; `qt6` via Homebrew on macOS).

```sh
sudo apt install qt6-base-dev   # Debian/Ubuntu; skip if already installed
cmake -B build -S .
cmake --build build --config Release
./build/desktop/pigmenta-desktop
```

If Qt6 isn't found, configure logs "skipping desktop/" and only `core/` +
`cli/` get built.

### CLI

No extra dependencies besides CMake >= 3.22 and a C++17 compiler.

```sh
cmake -B build -S .
cmake --build build --config Release
./build/cli/pigmenta-cli input.png output.png --filter decorrelation --colorspace lab --std 50
```

## Android app

```sh
cd android && ./gradlew assembleDebug
```

An APK will also be published to the Play Store for anyone who just wants the
app without building it themselves.

## Tests

Unit tests next to the code they test, 100% line coverage on `core/` — see
[docs/TESTING.md](docs/TESTING.md).

## Contributing

Contributions welcome — see [CONTRIBUTING.md](CONTRIBUTING.md).

## License

GPLv3 — see [LICENSE](LICENSE).
