# Local FlashVM control-point motion

[Русский](FLASHVM-LOCAL-MOTION.ru.md) · [Object/node tools](VECTOR-EDITING.md)

The recorded slow operation is a Nodes drag. Previously every native movement
went through the desktop controller, Limbo pane/model transform and complete
geometry/overlay serialization before FlashVM could draw the new position.
The render callback was already independent of SWF frames; that did not remove
the per-movement request/reply cost. Async.post also runs on the context owner,
not a separate OS thread. AS frontend recognition of asinc is not executable
closure/task lowering: the current FIR profile still refuses it explicitly.

## Implemented path

After Begin admission, the Limbo UI policy emits a copied modifier plan: source
index/revision, baseline pointer, selected point masks and explicit decoration
bindings. It is cached with the owner presentation. The private stream uses tool
flag0x40000; its bounded plan precedes the shape count. Masks have three bits per
source command; decoration bindings identify an endpoint/control plus optional
square radius/corner. No object/heap address or executable callback crosses.

`dis/bridge/idevectorlocal.b` constructs those values. `vector_local.c/.h` in the
native adapter applies the generic copied modifiers to its immutable baseline.
It updates selected endpoints/Bezier controls, welded closed endpoints, source
prefixes, guides and handle squares transactionally. Invalid movement preserves
the last valid geometry/pointer. Clipping is applied after marker offsets, so
page-edge squares retain their dimensions when moved inward. Repeated movement
uses the original snapshot and accumulates no rounding drift.

Actual native captured Nodes motion now stays on the FlashVM owner; it does not
send Motion to Dis. The existing frame-free callback requests detached raster
work. A local content generation invalidates the associated bitmap key, while
unchanged nodes keep normal reuse. Source/body bytes are not serialized per move.
An unrelated canonical snapshot reapplies the local position only when revision,
source, masks, bindings and baseline geometry match exactly. A changed/absent
plan retires the modifier. Escape/focus cancellation restores the baseline.
Keyboard/lifecycle barriers preserve the final valid point before their next
proposal where appropriate. Release crosses once for the authoritative model
commit and one Undo transaction. Partial owner snapshots queue input until logical End; they cannot remap a
captured pointer against incomplete arrays. Replay retains the full legacy route as an
independent reference, and new hits retain the presented-geometry barrier.

This is implemented for admitted Nodes translations, including grouped selected
points/controls. Initial selection and final document commit still use Dis.
Object move/scale/rotate, canvas/desktop zoom, rail panning, buttons and palettes
have not all been moved into local FlashVM actors. This checkpoint does not
claim the requested all-IDE AS asinc/thread architecture is complete.

## Execution target still open

Each executable asinc block must own a real worker thread and isolated FlashVM
context/heap, with copied/copy-on-write captures and the agreed dirty-only frame
synchronization. UI actors exchange values/commands with the sole MovieClip owner;
several threads must not mutate one VM heap. Persistent event loops may live in
one asinc block per actor. Async.post is only owner delivery, not that worker.
The compiler's closure/capture/task lowering, event binding and broader IDE ports
remain required; wrapping existing Dis callbacks in Async.post is insufficient.

## Evidence

The native SDL fixture queues600 real node motions and observes only Begin/End
proposals. It advances no SWF timeline and provides no intermediate document
reply. Actual source/handle pixels change before release acknowledgement; another
canonical snapshot cannot rewind the local position. Generic C tests cover
independent controls, masks/bindings, page clipping, invalid coordinates, immutable
baselines, rollback and welded endpoints. Real interpreter/JIT desktop replays
separately exercise the unchanged authoritative model path. These are executable
dummy-SDL/raster checks, not a physical platform speed guarantee.


Source interfaces: [copied modifier API](../disVM/runtime/vector_local.h),
[value-transform implementation](../disVM/runtime/vector_local.c),
[desktop integration](../disVM/runtime/desktop_vector_local.c),
[owner plan encoder](dis/bridge/idevectorlocal.b),
[native SDL pixel test](../disVM/runtime/test_local_motion.c).
