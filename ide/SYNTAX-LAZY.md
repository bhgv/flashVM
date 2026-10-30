# Lazy lexical catalogs for Limbo and ActionScript

[Русский](SYNTAX-LAZY.ru.md) · [IDE entry points](ENTRYPOINTS.md) ·
[Desktop](DESKTOP.md) · [Compiler frontends](../compiler/README.md)

Implemented: a portable Limbo/Dis analysis worker, a coalescing lazy owner,
Coco/R-generated scanners and **visible mixed-colour text in desktop editors**
(menu8). The FlashVM native range-contour extension shares whole-field shaping;
AS mixed-range TextFormat/HTML and mixed fonts remain outside this implementation.
[Native contour contract](../flashVM/doc/TEXT_COLOR_RANGES.md).

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

## Incremental reuse and ownership

The worker compares source scalars with one successful same-profile baseline.
Only complete prefix records ending strictly before the first change are retained.
The token ending exactly at that boundary is rescanned: identifiers, numbers and
comment openers can merge across it. A preceding class5 gap is also rescanned,
because generated whitespace/comment gaps can join. Restart is before a complete
old token, never inside an old string/comment. Generated tables and matching stay
unchanged; the facade checks the restart byte boundary and initializes read-only
tables once per private instance.

Scanning stops early only after a generated record lies wholly in the unchanged
suffix and matches an old record's class and shifted scalar extent. The remaining
tail is then copied with the scalar delta. Otherwise scanning continues to EOF.
Profile changes force a full pass; identical validated source can reuse everything.
Errors/quotas publish no partial catalog and retain the last successful baseline.
Multiline/incomplete string/comment changes can still require a large/full pass.

`Result.scanned` counts returned generated records excluding EOF, `reused` counts
copied records, and `restart` gives the scalar restart (source length for unchanged
reuse). On success scanned+reused equals catalog size. These are record counters,
not timings/opcode counts: the facade can prefetch a token while returning trivia.

Private baselines are never delivered. Worker results and lazy Snapshot replies
contain detached arrays, so receiver mutation cannot corrupt later analysis or
another snapshot. Consumers should keep their catalog unchanged while querying.
Retained state is bounded: one successful baseline, one active job, one newest
pending source and bounded delivered/owner catalogs; no edit history accumulates.
Scalar validation, byte-boundary construction and copying still do linear work.
This is not an incremental semantic parser or a physical latency guarantee.

[idesyntaxincremental_test.b](dis/tests/idesyntaxincremental_test.b) compares edited
catalogs against a separate fresh worker running the real generated scanner.
It covers merges, comment delimiters, escapes/incomplete text, profile/Unicode
changes, receiver mutation and failure recovery. A local edit in a96-line fixture
scans at most3 returned records and reuses over600; both VM modes are checked.

## Desktop presentation

[Idedeskpane](dis/bridge/idedeskpane.m) keeps document ownership and optionally
starts a private lazy analysis owner through `startnotified`. A buffered readiness
channel and positive pane identity enable this work; plain `start` preserves the
old library/document path without background scans. Untitled/.b source uses Limbo;
.as/.as1/.as2/.as3 uses the AS lexical profile. Unknown suffixes and missing
optional scanner dependencies remain plain text. Failed/over-budget snapshots
remain editable without partial colours. The newest snapshot now reuses safe
records from a private last-success baseline, as described below.

A pane projects its current revision/profile into the <=200-scalar viewport.
Completion notifications contain only stable pane identities. The app fetches a
fresh snapshot and compares revision, viewport origin and body before publishing.
Hidden panes retain their catalog without painting; reopening uses a fresh view.
The [app](dis/app/desktop.b) sends `ColorsView` (local UI code65575) through the
existing typed channel, containing the completed view ticket, pane ID, bounded
source revision and ASCII palette. Captions replace control/line separators for presentation only; actual paths remain intact.
The source revision is authoritative at the
Limbo owner; native admission checks the view ticket, live editor item, scalar
count, quota and palette classes. It does not independently own source revisions.
This code is unrelated to shared Dis64 wire/extension IDs.

A staged palette belongs to its full scene. A later paint-only palette keeps the
same geometry/input ticket and cannot acknowledge queued keyboard input. It
neither moves a window nor changes source/Undo/save/caret/selection. On the VM
owner, native code converts scalar colours to byte intervals and exports detached
contours per palette class from one cached layout. Names keep the default ink;
numbers, strings, keywords and comments get distinct colours. Selection is drawn
first and caret last. Thumbnail copies use the same source palette. Drag snapshots
and contour caches include palette values in their invalidation keys.

The existing immutable render jobs and dirty tiles perform raster work separately
from the VM; changed palettes use that same damage path. The owner still performs
layout/contour preparation. This does not claim pixel-minimal damage for every
edit, physical latency measurements, or OS-thread-parallel Dis/GC execution.
[test_desktop_syntax.py](../disVM/runtime/test_desktop_syntax.py) checks actual
RGB ink, unchanged saved source/dirty state for colour-only updates, then a burst
of26 queued characters while results compete with input. Native protocol tests
also distinguish paint-only completion from a complete-scene input acknowledgement.

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
latency or parallel VM execution. Real headless MovieClip/raster integration
checks additionally verify coloured pixels and queued edits in both VM modes.
