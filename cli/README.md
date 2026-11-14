# Pigmenta CLI

Batch image enhancement on top of `pigmenta_core` — no Android required.

## Build

```sh
cmake -B build -S .
cmake --build build --config Release
```

Or standalone with just a compiler (no CMake, no external deps beyond the
vendored `stb_image` headers in `third_party/stb/`):

```sh
g++ -std=c++17 -O2 -I../core/include -Ithird_party/stb \
  ../core/src/*.cpp src/main.cpp -o pigmenta-cli
```

## Usage

```sh
pigmenta-cli input.png output.png --filter decorrelation --colorspace lab --std 50 --intensity 1.0
pigmenta-cli input.png output.png --filter clahe --tile 16 --clip 2.0
pigmenta-cli input.png output.png --filter gamma --gamma 0.7
pigmenta-cli input.png output.png --filter saturation --factor 2.5
pigmenta-cli input.png output.png --filter relief --angle 315 --strength 0.5
pigmenta-cli input.png output.png --filter ir --intensity 0.8
```

Run with no arguments for full usage.
