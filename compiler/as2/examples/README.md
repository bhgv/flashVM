# AS2 syntax examples

[Русский](README.ru.md). These examples exercise the broad syntax frontend,
not the separate numeric FIR backend. No SWO, ABC, Dis or SWF is emitted here.

| File | Purpose |
| --- | --- |
| [in.as](in.as) | Ordinary membership, for-in separator and parenthesized NoIn initializer |

Build from this language directory using the explicit external HOST Coco/R:

```sh
cmake -S . -B build -DCOCOR_ROOT=/host/coco-r-C \
  -DCOCOR_EXECUTABLE=/host/coco-r-C/sources/cocor
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
build/frontend/as2_parse examples/in.as
```

The driver returns 0 for accepted syntax, 1 for syntax/encoding rejection and 2
for usage/I/O failure. It does not execute trace, send channel values or resolve
classes/types. AS examples now also demonstrate automatic semicolons. The Limbo fixture
uses native # comments and intentionally spaced wildcard arrows. Libraries
accept caller-owned UTF-8 bytes/streams, copy no source ownership and support
sequential reuse after errors; serialize calls to each non-reentrant parser.

[Language/API coverage](../README.md), [authoritative frontend](../frontend/README.md),
[working numeric FIR subset](../../../fir/v1/doc/SOURCE_SYNTAX.md),
[native/cross build guide](../../../BUILDING.md),
[all compiler examples](../../examples.md). Cross-built drivers run on their
selected OS/CPU; the command above is a native run, not an emulator recipe.

## Extended parser example

[Automatic.as](Automatic.as) exercises syntax only, without execution or object emission.
The example checks readable source through the same C API as the CLI driver.
The host owns input bytes/FILE and diagnostic storage; parsing closes nothing
and transfers no ownership. Includes, types, symbols and events are not executed.
Every nonempty example line has an explanatory comment. Expected result: status 0
and no generated file. Syntax rejection is status 1; usage/I/O failure is status 2.

```sh
build/frontend/as2_parse examples/Automatic.as
```

[Grammar and API contract](../README.md) · [Build](../../../BUILDING.md)
