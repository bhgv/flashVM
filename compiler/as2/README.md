# AS2

Version 0.2.4. [Русский](README.ru.md).

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

`frontend/frontend.h`: `as2_check(bytes, size, diagnostic)` and
`as2_read(FILE*, diagnostic)` return 1 accepted / 0 rejected. The caller owns
the input buffer/stream; the library prints nothing, closes no input and never
exits. Diagnostic fields: code, line/column, byte offset, message. NULL diagnostics
are allowed. Maximum input: 2 MiB. CMake target: `as2_frontend`. Generated global
symbols have a language prefix; multiple libraries can link into one IDE. State
resets before every call, but each parser requires external serialization (not
reentrant). `tests/embed.c` checks in-process recovery after invalid input.

### in operator

Relational `in` and for-in/classic-for disambiguation are supported. The initial
for clause uses NoIn; groups, arguments, subscripts and literals locally restore
ordinary expressions. Function bodies own their expression context. Productions:
Relational, ForStatement, GroupedExpression, Arguments, Subscript. This is syntax
recognition, not property enumeration or FIR lowering.

[Grammar](frontend/as2.atg): Relational/ForStatement. [Example](examples/in.as).

### C API parsing limits

Recursive grammar boundaries are limited to 512 active calls and 4000000 visits.
Exhaustion reports diagnostic.code -3/-4 instead of terminating the IDE; the next
file can still be parsed. Counters and Coco/R state reset before each call.
Resource-failure coordinates may be zero. Limits do not count parentheses:
one expression level traverses several productions. This is an implementation
bound, not a language-syntax change. Parsing stays authoritative in .atg; guards
are C99.
