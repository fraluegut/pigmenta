# Pigmenta Desktop

Qt6 Widgets GUI. Dark, minimalist, Meshroom-ish panel layout — drop an image,
pick a filter, adjust its parameters live, compare before/after.

## Features

- Drag-and-drop (or click-to-browse) image loading.
- Filter list with live parameter sliders (decorrelation stretch, CLAHE,
  gamma, saturation, relief shading, IR simulation) — same math as the
  Android app and the CLI, via `pigmenta_core` linked directly (no subprocess).
- Compare view with four modes: **After**, **Before**, **Wipe** (draggable
  split line) and **Side by side**.
- Export the processed image to PNG/JPEG.

## Build

Requires Qt6 (`Widgets` module) and CMake >= 3.22.

```sh
# Debian/Ubuntu
sudo apt-get install qt6-base-dev cmake

cmake -B build -S ..   # from desktop/, builds core/ + desktop/ together
cmake --build build
./build/desktop/pigmenta-desktop
```

Or build everything from the repo root — `desktop/` is included automatically
when Qt6 is found:

```sh
cmake -B build -S .
cmake --build build
./build/desktop/pigmenta-desktop
```

If Qt6 isn't installed, the root build just skips `desktop/` and still builds
`core/` + `cli/`.
