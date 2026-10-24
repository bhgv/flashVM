# Numeric Limbo lexical blocks, FIR tools 0.5.3

[Русский](LEXICAL_SCOPES.ru.md) · [Source profile](SOURCE_SYNTAX.md) ·
[Commented example](../examples/limbo/scopes.b)

The executable Limbo source adapter now accepts nested `{ ... }` statements.
Declarations belong to their block. A local declaration can shadow an outer
name; its initializer reads the previously visible outer binding. Duplicate
initialized declarations in the same block remain errors. Local names disappear
at block exit. Assignment targets the nearest visible initialized name and
persists when that name belongs to an outer block. AS source profiles retain
flat bindings and do not gain braces from this change.

The compiler needs no FIR phi or mutable-slot extension: every block executes
once in source order, and binding-table updates select the appropriate SSA value.
This does not implement branches, loops, early returns, functions, channels or
general Limbo modules. Return remains the final statement of the numeric value()
entry. No full-language compatibility is implied by the shadowing rule.

All blocks, parentheses and general unary operators share a 64-level nesting
budget. Retired names still occupy their bounded translation-unit slots:
256 slots total, including shadowed names and references. This explicit bound
prevents unbounded metadata growth; the existing 1000-SSA and 64-KiB limits apply.
An over-budget opening brace is consumed and rejected without recursive descent,
avoiding a nonadvancing recovery loop. Every new parse resets recursion state,
including after an incomplete nested input in an embedding process.

## Reproduce

Build tools as in the [pipeline](PIPELINE.md); from the workspace root:

```sh
# Compile the fully commented example through the authoritative Coco/R grammar.
build/fir1/fir1_source limbo --input fir/v1/examples/limbo/scopes.b scopes.fir
# Validate dominance/types before optimization.
build/fir1/fir1 check scopes.fir
# Fold integer operations and inspect the final const.i32 48 / trace pair.
build/fir1/fir1 opt scopes.fir scopes.opt.fir
# Emit a separate object; link it using the independently built SWL.
build/fir1/fir1 object scopes.opt.fir scopes.swo
build/swl/swl link scopes.swf scopes.swo
```

Use a writable TMPDIR on hosts where the default temporary directory is unavailable.
The expected trace is `48` followed by newline. Producing/inspecting FIR is not
proof of executing the movie; use an explicit runner for runtime acceptance.
The registered `fir1_source_scopes` regression checks eight valid lowering/
optimization cases and nine rejected inputs with destination rollback. Existing
embedding tests also recover from incomplete parenthesized input in one process.
