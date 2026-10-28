# LIMBO

Version 0.10.0. [Русский](README.ru.md).

Independent C99 compiler project. The current implementation is a syntax
frontend with owned concrete production trees: [grammar and build](frontend/README.md),
[tree API and limits](frontend/TREE.md). Full semantic analysis, typed AST construction,
bytecode and SWF emission are not implemented yet.

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

### Syntax 0.3.5

Added list-of literals, trailing array-initializer commas, unary dereference `*`
and labels on while/do/for/case/alt/pick. Typed declarations after a colon do
not become labels. `a[]` fails; v0.4.2 additionally accepts `a[:hi]` and `a[:]`. `array[] of Type` reports E1301: omitted size needs a nonempty initializer.
Use nil for an empty list, not `list of {}`. Explicit lookahead distinguishes
wildcard `* =>` from dereference `*p`.

[Grammar](frontend/limbo.atg): LabeledStatement, ArrayCreation, InitializerList,
Qualifier, Subscript, Unary. [Example](examples/Syntax.b). Based on
[Limbo reference sections 8 and 9](https://www.inferno-os.org/inferno/papers/limbo.html).
Label scopes, list/channel types and alt/pick execution are not checked. This
is not FIR lowering. The C API retains recovery after invalid input.

### C API parsing limits

Recursive grammar boundaries are limited to 512 active calls and 4000000 visits.
Exhaustion reports diagnostic.code -3/-4 instead of terminating the IDE; the next
file can still be parsed. Counters and Coco/R state reset before each call.
Resource-failure coordinates may be zero. Limits do not count parentheses:
one expression level traverses several productions. This is an implementation
bound, not a language-syntax change. Parsing stays authoritative in .atg; guards
are C99.

[Detailed examples](examples/README.md).

## Syntax 0.4.2

Adds multiple implement modules, polymorphic function/ADT/type parameters and
for constraints, exception declarations/raises/raise/block handlers, fixed types
and conversions, **/**=, array of byte conversion, trailing list comma and omitted
slice bounds. Variadic * must be last (E1302); empty case/alt/pick and empty for
constraints are rejected. Comparison with current Inferno corrected the former
lower-bound restriction: a[:n] and a[:] are legal.
Includes are not expanded. Named and instantiated type casts are recognized. Symbol/type resolution and
complete type/value disambiguation in polymorphic arguments still need work. This is not a complete
Inferno compiler or Dis generator.

[Polymorphic example](examples/Polymorphic.b), [upstream grammar](https://github.com/inferno-os/inferno-os/blob/master/limbo/limbo.y).

[Lexical profile 0.6.0](../LEXICAL_PROFILE.md).

[Independent .sbl source-map reader](symbols/README.md) · [C/Limbo/CocoR compiler direction](symbols/COMPILER_DIRECTION.md).

## Explicit interface declarations

The [C99 interface catalog](frontend/INTERFACES.md) now owns module/export identities,
grouped names and authored type spans, with exact scope-name duplicate checks.
This partial declaration pass does not resolve imports/types or emit native C.

[Own compiler priority / Приоритет Limbo](../../compiler/limbo/OWN_COMPILER.md) ·
[Two-way .m external AS/Dis modules / Внешние модули](../../compiler/limbo/examples/external-modules/README.md) ·
[Owned tools / Инструменты](../../tools/UPSTREAMS.md).

[Structured function signatures / Структурированные сигнатуры](frontend/SIGNATURES.md)

[Our numeric Limbo→FIR→Dis pipeline](../../fir/v1/doc/DIS_BACKEND.md) now executes
on DisVM without a bootstrap emitter. It is a separate narrow profile; this
syntax frontend has not become a full semantic compiler.

[Native C interfaces from .m](frontend/NATIVE_C.md) / [Русский](frontend/NATIVE_C.ru.md).

[Outer alias resolution / Внешние алиасы](frontend/ALIASES.md) · [RU](frontend/ALIASES.ru.md).
