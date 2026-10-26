# Source compilation with debug origins (FIR 0.5.5)

[Русский](SOURCE_DEBUG.ru.md) · [Diagnostics](SOURCE_DIAGNOSTICS.md) ·
[Own Limbo compiler](../../../compiler/limbo/OWN_COMPILER.md)

Our Coco/R source compiler accepts `-g DEBUG.json`, including the numeric Limbo
profile. This is our debug-producing compilation path, separate from the legacy
native compiler's mature Dis/.sbl generator. For example:

```sh
./build-workspace.sh --projects fir
export TMPDIR="$PWD/build/workspace/tmp"
build/workspace/fir/fir1_source limbo --input fir/v1/examples/limbo/scopes.b build/workspace/scopes.fir -g build/workspace/scopes.debug.json
```

`--module`, `--diagnostics-json` and `-g` may be ordered after the input/output
arguments; duplicates/incomplete options are rejected. The additive C99 API is
`fir_source_debug`, taking the same diagnostic arguments as fir_source_ex followed
by an optional caller-owned debug FILE*. Existing APIs call it without debug output.
The three streams must be disjoint; the exact same input/output/debug stream is
rejected. Parser calls remain externally serialized. No borrowed FILE/source/error
pointer survives return, including failure and a following plain compilation.

## FIRDBG1

The JSON contains schema=1, format=FIRDBG1, stage=source-unoptimized-fir, module,
the exact admitted UTF-8 source snapshot and an ordered events array. Events record:

- constant/binary: the emitted `%vN` origin; `value` stores N.
- declare/assign: source name and the current SSA binding.
- trace: its input SSA value.
- scope-enter/scope-leave: lexical lifetime boundaries, value=-1.

`scope` is a translation-unit-local unique lexical owner ID, with 0 for the entry
scope. Sibling blocks have distinct IDs even at the same scope_depth. Nesting can
be reconstructed from ordered enter/leave events. Variable assignment records the
actual binding owner, including writes to an outer variable from an inner block;
expression origins use the current lexical owner. IDs reset for each compilation.
Positions are original zero-based UTF-8 byte_offset/byte_length and one-based
physical line/byte column. They are not character/display columns. Source token
ranges, including operator/target captures, are shared with structured diagnostics.
Unary-minus helper constants are also recorded; every emitted SSA definition has
an origin. No native pointer, GC root or private compiler address is serialized.

FIR and debug JSON are privately staged. Syntax/semantic/encoding rejection writes
neither output stream and the CLI preserves both prior files. Final stream/file
I/O can leave a prefix or one published file; this is not a two-file atomic
transaction. The caller owns and closes all streams.

## Important stage boundary

This sidecar maps **unoptimized FIR source identities**. Optimizer rewrites,
SWO/SWF linking and final Dis/native JIT PC relocation are not represented yet.
Do not treat `%vN` or byte_offset as a machine address or breakpoint PC. Reusing
this map after optimization without relocation is invalid. General function/local
variable debug layouts remain part of the full Limbo compiler/backend work.
The explicit stage field prevents advertising this as completed end-to-end debugging.

Tests use real Coco/R compilation, check SSA coverage, unique sibling scopes,
outer assignments, UTF-8 positions, retained outputs, sequential API reset and
closed-debug-stream retirement. The native bootstrap separately supports -g/.sbl;
its output is not presented as our own complete Dis backend.


### Occurrence identity (additive FIRDBG1 fields)

Each event includes a positive `site` unique within the staged debug artifact and
`binding` (positive lexical declaration identity, or zero when not applicable).
`read`, `declare` and `assign` events distinguish repeated identifier occurrences
without changing existing SSA IDs. Compound assignments record their implicit
read separately. Coordinates remain UTF-8 byte offsets/columns. Combine site
with artifact/build identity; it is not a final Dis/native PC or global ID.
See the [broker contract](../../../disVM/broker/README.md).
