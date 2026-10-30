# Lazy lexical catalogs for Limbo and ActionScript

[Русский](SYNTAX-LAZY.ru.md) · [IDE entry points](ENTRYPOINTS.md) ·
[Desktop](DESKTOP.md) · [Compiler frontends](../compiler/README.md)

Implemented: a portable Limbo/Dis analysis worker, a coalescing lazy owner and
Coco/R-generated scanners. **Visible mixed-colour editor text is not connected
by this series.** The existing Flash TextField uses uniform formatting; arbitrary
AS range TextFormat/HTML support is not added here. Rendering colour ranges is
the next integration, separate from the tested lexical service.

The source of recognition is [limbolex.atg](syntax/limbolex.atg) and
[aslex.atg](syntax/aslex.atg), built by our selected Coco/R Limbo generator.
[generate.py](syntax/generate.py) adds only reset/next exports and suppresses the
scanner's synchronous incomplete-input diagnostic. Recognition tables and input
matching remain generated. Scanner outputs live in the build directory.
The older [AS worker prototype](as/syntax-worker.as) remains independent and is
not used as a fallback or the authoritative source of these catalogs.

## Contracts and ownership

[idesyntax.m](dis/bridge/idesyntax.m) defines copied Request/Result/Token values
and the lazy Command interface; [idesyntax.b](dis/bridge/idesyntax.b) implements
both owners. One generated module instance per profile belongs to each worker.
This is a scheduled Dis process, independent of Flash timeline frames; it does
not claim parallel execution of multiple VM owners on OS threads. Native raster
workers remain a separate immutable-job pipeline.

`start` returns unbuffered job/result channels. For every accepted job, receive
one result before submitting another; stop also has an acknowledgement. Missing
scanner modules return nil channels, never a handwritten fallback.

`startlazy` returns an owner that acknowledges Update/Snapshot without waiting
for analysis completion. There is at most one active immutable scan and one
newest pending snapshot. Each Update requires a strictly increasing positive
ticket, a nonnegative revision and a supported profile. The owner immediately
invalidates the old presentation catalog. Old ticket/revision/profile results
cannot replace the latest catalog. Shutdown drains active bounded work and
acknowledges termination; it can wait for that work. Clients must receive replies.
Optional buffered notices are best effort; Snapshot is authoritative. Nil notices
are supported without selecting on a nil channel.

A catalog covers the complete admitted snapshot in ordered half-open Unicode
scalar offsets. UTF-8 continuation boundaries never escape. Classes are:

| Class | Meaning |
| --- | --- |
| 0 | Punctuation or unclassified generated token |
| 1 | Name, including non-ASCII lexical names |
| 2 | Tolerant numeric spelling |
| 3 | Quoted string/rune, including incomplete editing input |
| 4 | Keyword |
| 5 | Scanner-skipped trivia, including comments and whitespace |

Budgets: 65,536 scalars, 262,144 UTF-8 bytes and 8,192 records per snapshot.
Surrogates/non-scalars fail validation. A quota/error returns no partial success.
These are service resource limits, not language/compiler limits. `project`
binary-searches an authentic catalog and visits intersecting records for at most
200 visible scalars. A revision/profile mismatch returns an empty palette. It
never changes source, Undo, save state, caret, selection or view geometry.

## Reuse and limits

Consumers can retain the same immutable token catalog for lexical context,
search exclusion/inclusion and future completion candidates. Positions are source
coordinates, not pixel coordinates or editor view tickets. Declaration resolution,
typed completion, scopes, diagnostics and FIR lowering still belong to compiler
frontends. These tolerant editor grammars are not substitutes for full compiler
parsers. AS regular-expression/E4X context, complete numeric spelling and full
Unicode identifier validation are not implemented. Incomplete strings/comments
remain colourable lexical data; the compiler remains authoritative for errors.

## Build, run and evidence

The runtime builds Coco/R from explicit `DVM_COCOR_LIMBO_ROOT` (default selected
`tools/coco-r-limbo`), generates both scanners and compiles all Dis modules using
its matched native-cell bootstrap. No generator/Python is required to run the
installed service. The temporary legacy bootstrap is not our primary compiler.

After building/installing disvm-runtime or disvm-editor, final menu **16** or
`--launch syntax` runs a real service check; it does not open a coloured GUI:

```sh
python3 build-menu.py --projects disvm-runtime --backend cmake --launch syntax
sh out/disvm-runtime/run-syntax-check.sh --no-jit
sh out/disvm-runtime/run-syntax-check.sh --jit
```

[idesyntax_test.b](dis/tests/idesyntax_test.b) uses actual generated scanners and
Dis channels. It checks escaped/incomplete strings/comments, supplementary
Unicode, atomic quotas, immutable old catalogs, stale projection, an edit burst
while old analysis is active, duplicate tickets and shutdown. Both interpreter
and JIT pass. These are functional scheduling tests, not measured physical typing
latency, parallel VM execution or completed GUI colour acceptance.
