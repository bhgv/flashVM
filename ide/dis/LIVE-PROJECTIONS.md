# Live document projections and the graphical inspector

[Русский](LIVE-PROJECTIONS.ru.md) · [Values](VALUES.md) ·
[Projection model](PROJECTIONS.md) · [Runnable example](examples/document-projection/README.md)

## Actual owner, no duplicate document

`Idedocprojection` borrows the real `Idecore` request channel. It never receives
the document's private ADT and does not maintain a second source/history store.
ReadAs snapshots the actual owner and converts one declared field using Idevalue.
WriteValue forwards a typed replacement to that owner. InvokeRef calls its real
Undo/Redo operations. The adapter has its own Dis process; only that process waits
for document replies. It is not a generic debugger heap-reading facility.

The reflected document members are text:string, length:int, revision:int,
saved:int, dirty:bool, self:Object, and the bound undo/redo methods. Length counts
Limbo Unicode characters, not UTF-8 bytes, UTF-16 units or pixels. Text follows
the existing 4096-character scalar copy quota and fails as a whole when too large.
The metadata table contains length/revision/saved/dirty/self and remains bounded
even when the document is much larger. It contains no executable methods/getters.

A document mutation uses `request.number` as its explicit expected revision;
this is this adapter's method argument convention, not a new meaning for all
projection owners. The adapter checks its current snapshot and passes the same
revision to Idecore, which checks again atomically with the mutation. A source
change between these two rendezvous therefore returns Stale rather than editing
the wrong revision. InvokeRef additionally checks receiver identity, incarnation
and explicit permit. No failed/timed-out mutation is automatically retried.

Adapter Pause prohibits its own writes/invocations; it does not suspend the
ordinary editor or other clients of the document owner. Reads remain available.
Release consumes one acknowledgement and stops only the adapter. Always release
before shutting down the borrowed document service, then retire the endpoint.
Identity is an adapter-session identity, not the currently selected file path;
a controller must replace it when switching to another document service.

## Console IDE

The running editor accepts:

```text
watch length big
watch text string
watch self object
watch undo method
watchtable
callref undo 2
setref 3 replacement text
```

Type names come from `Idevalue.types()`. The example revision numbers must be
replaced with the revision observed from the actual document. `callref` obtains
a bound method descriptor and invokes it only after that explicit command;
expanding or printing a watch does not execute the method. `setref` accepts the
same checked text escapes as normal source editing. All replies report their
actual status/revision. Ordinary `show`, dirty-state checks and save/history
continue to use the same authoritative document owner.

## Graphical FlashVM presentation

Ctrl+I opens a read-only document inspector. The Limbo controller obtains its
metadata through the same live projection channel and sends copied presentation
text through the normal UI envelope. Esc or Ctrl+I returns to editing. The panel
shows length, revision, saved revision and dirty state, refreshing from the real
owner. Input text, Delete, Cut/Paste and Undo cannot mutate source while the
read-only panel is active. Resize/zoom and explicit Quit still use their checked
controller paths. Font default 11 and packaged Noto remain unchanged.

Native Panel mode 7 is presentation-only: it accepts counted UTF-8 and renders
its label/value without a caret. It does not query a document, evaluate a
variable, invoke a method or change application state. Older renderers that only
admit modes 0..6 must be updated together with the controller. This adds a
read-only document panel, not yet an arbitrary user-program locals/watch tree.

## Validation and ownership repair

The example and console tests run on actual Dis interpreter/AMD64 JIT and check
native dispatch counters. They cover Unicode, scalar/table copies, a stale
revision, bound Undo, consent, copied assignment, pause, large-source quota and
release without stopping the document. Installed packages recompile/run the
same example after relocation using shipped headers/modules.

The graphical test replays input through the real Dis controller and FlashVM
renderer, checks returned metadata and unchanged source/history, then compares
actual raster bytes with a plain editor view. It checks closing/editing/reopening
and Ctrl+I toggling. This is headless raster acceptance; it is not a claim of
physical SDL/touch testing in this series.

The panel exposed a production conversion defect: Limbo can reuse a pointer
temporary holding a Value ADT as the destination of `string big`. Numeric
formatting previously presumed that old owner was String. The owning Inferno
fix captures the scalar first, reuses only exact Tstring owners, allocates before
releasing other owners, and adds write-free AMD64/ARM64 fallback guards. C/RX,
allocation-failure, shared-owner and emulated wrong-Type regressions accompany
that upstream fix. The panel now exercises the repaired path directly.

Network RPC, authentication/export GC, generic `any` lowering, arbitrary FlashVM
object reflection and full debugger safe-point integration remain separate work.
The channel/ADT modules currently use the matching source-built Limbo bootstrap;
our Coco/R numeric FIR frontend is not presented as their compiler.


[Window buttons and line gutter](WINDOWS-GUTTER.md) extend this presentation with
a visible dirty-close choice (mode 8), checked window resize notifications and
Ctrl+L line numbers. Native/controller interfaces must be upgraded together.

## Static types and owner requests

The live document already serves a typed `chan of ref Ideprojection->Request`.
This is an explicit request/reply protocol; ordinary Limbo variables do not
automatically become remote variables. Each ReadAs selects asType, obtains a
current owner snapshot and performs a checked conversion. Reading length as Int
returns an integer; reading it as Text returns a string. An incompatible value
returns Invalid and nil. Another client may mutate between reads; an earlier
copied snapshot stays unchanged.

Standard Limbo has no built-in dictionary. ADTs describe known structures; a
library can implement dynamic name tables. Value.Table is a bounded shallow
snapshot with unique names, not a language dictionary. Objects with methods stay
at their owner: Object/Method carry identity and incarnation, and operations use
the adapter channel. Those descriptors contain no endpoint and are not network
capabilities by themselves. Within one Dis VM, ref ADTs can share memory according
to Limbo semantics; such pointers must never cross VM heaps or a network.

Compiler insertion of ReadAs from expression types, arbitrary FlashVM variable
exports, distributed cycle collection and network transport remain unimplemented.
Copying a value, retaining an object proxy and constructing a new object from a
snapshot must stay explicit separate operations. The document-projection test
covers owner changes between reads, two requested types and conversion failure.

FlashVM currently stores FVM_Object named/indexed properties in separate maps. The standalone hybrid_table.hpp is not connected to FVM_Object; no shared Lua-derived universal table module is exposed to Limbo. Runtime native_tables.c implements Dis switch/case tables, not a user dictionary.
