# AS1

Version 0.2.0. [Русский](README.ru.md).

Independent C99 compiler project. The current implementation is a syntax
frontend: [grammar, build and limitations](frontend/README.md). Semantic
analysis, AST construction, bytecode and SWF emission are not implemented yet.

Configure this directory with the same explicit tool paths as in the frontend guide:

```sh
cmake -S . -B build \
  -DCOCOR_ROOT=/path/to/coco-r-C \
  -DCOCOR_EXECUTABLE=/path/to/coco-r-C/cocor
cmake --build build
ctest --test-dir build --output-on-failure
```

CLI: `--help`, `--version` (no source file required; status 0).

## C API

`frontend/frontend.h`: `as1_check(bytes, size, diagnostic)` and
`as1_read(FILE*, diagnostic)` return 1 accepted / 0 rejected. The caller owns
the input buffer/stream; the library prints nothing, closes no input and never
exits. Diagnostic fields: code, line/column, byte offset, message. NULL diagnostics
are allowed. Maximum input: 2 MiB. CMake target: `as1_frontend`. Generated global
symbols have a language prefix; multiple libraries can link into one IDE. State
resets before every call, but each parser requires external serialization (not
reentrant). `tests/embed.c` checks in-process recovery after invalid input.
