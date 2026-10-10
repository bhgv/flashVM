# FlashVM

Portable SWF 1-8 / AVM1 runtime project (early scaffold).

## Compatibility policy

- Public headers: C89-compatible C ABI; no STL or C++ types.
- Core: C++98; no C++11+ features, no OS-specific dependencies in core.
- Optional backends: rendering, media, JIT, platform windowing.
- Native extension interfaces will be versioned and GC-safe.
- No claim of SWF playback or ActionScript execution yet.

## Build

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure

For older CMake versions without `-S` / `-B`, use `mkdir build && cd build && cmake ..`.

## Current status

Only runtime/context creation and ABI/configuration smoke tests are implemented.
SWF parsing, AVM1, JIT, rendering, media, extensions and player are planned.
Unimplemented entry points return `FVM_UNSUPPORTED`.
