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

## Current implementation status

The portable C89-facing API supports runtime/context lifecycle, basic tagged
values, context-owned objects, object properties, globals and registered native
callbacks. `include/flashvm/value.h` declares these operations.

This is an early prototype, **not yet a SWF interpreter**. No AVM1 bytecode,
SWF parsing, JIT, renderer or media support is implemented. Object and string
storage currently lasts until context destruction; there is no garbage
collector. API object/string references must not outlive their context, and
contexts are not thread-safe. `fvm_load_memory` and `fvm_advance` return
`FVM_UNSUPPORTED`. Avoid untrusted native extensions; no sandbox is provided.

Build and test:

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure
