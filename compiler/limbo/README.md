# LIMBO

Version 0.3.0. [Русский](README.ru.md).

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

`frontend/frontend.h`: `limbo_check(bytes, size, diagnostic)` and
`limbo_read(FILE*, diagnostic)` return 1 accepted / 0 rejected. The caller owns
the input buffer/stream; the library prints nothing, closes no input and never
exits. Diagnostic fields: code, line/column, byte offset, message. NULL diagnostics
are allowed. Maximum input: 2 MiB. CMake target: `limbo_frontend`. Generated global
symbols have a language prefix; multiple libraries can link into one IDE. State
resets before every call, but each parser requires external serialization (not
reentrant). `tests/embed.c` checks in-process recovery after invalid input.

### Syntax 0.3.0

Added list-of literals, trailing array-initializer commas, unary dereference `*`
and labels on while/do/for/case/alt/pick. Typed declarations after a colon do
not become labels. `a[lo:hi]`/`a[lo:]` require a lower bound; `a[]` and `a[:hi]`
fail. `array[] of Type` reports E1301: omitted size needs a nonempty initializer.
Use nil for an empty list, not `list of {}`. Explicit lookahead distinguishes
wildcard `* =>` from dereference `*p`.

[Grammar](frontend/limbo.atg): LabeledStatement, ArrayCreation, InitializerList,
Qualifier, Subscript, Unary. [Example](examples/Syntax.b). Based on
[Limbo reference sections 8 and 9](https://www.inferno-os.org/inferno/papers/limbo.html).
Label scopes, list/channel types and alt/pick execution are not checked. This
is not FIR lowering. The C API retains recovery after invalid input.
