# Architecture

Pigmenta is a monorepo with a shared C++ engine and multiple front ends.

```
core/        Pure C++ image-processing engine (color spaces, decorrelation
             stretch, CLAHE, gamma, saturation, relief shading, IR simulation).
             No platform dependency — operates on raw ARGB pixel buffers.

bindings/
  jni/       JNI bridge exposing core/ to the Android app (Bitmap <-> buffer).

cli/         Command-line tool. Links core/ directly, decodes/encodes images
             with stb_image (vendored, public-domain/MIT, in cli/third_party).

android/     Android app (Kotlin, Jetpack). Self-contained Gradle project;
             its CMake build compiles core/ + bindings/jni into libpigmenta-engine.so.

desktop/     Qt6 Widgets GUI. Links core/ directly (static lib), no
             subprocess — drop/load an image, adjust filter parameters live,
             compare before/after/wipe/side-by-side, export.
```

## Why one repo

All front ends (mobile, CLI, desktop) share the exact same enhancement
algorithms in `core/`. Keeping them together means a fix or new filter in
`core/` benefits every platform immediately, with one source of truth and one
version number for the algorithms. Each front end is still an independent,
separately buildable project (own Gradle root or CMake target) — this is a
monorepo of independent packages, not a single entangled build.

## Adding a new filter

1. Implement it in `core/src/filters.cpp` (or a new `.cpp`), declare it in
   `core/include/pigmenta/filters.h`.
2. Expose it over JNI in `bindings/jni/pigmenta_jni.cpp` and the Kotlin side
   (`android/app/src/main/java/com/pigmenta/app/engine/PigmentaNative.kt`).
3. Add a `--filter` case in `cli/src/main.cpp`.

No changes to the underlying math are ever duplicated between platforms.
