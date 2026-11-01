# Independent geometry senders and brokers

IDE 0.7.4 / workspace v0.1715 uses one `Idebulk` Limbo process and one native
`DVM_UIBridge`/`DVM_UIPump` instance per vector pane (stable IDs 1..8).
The ordinary input/control broker never carries geometry fragments from this path.
This is private IDE transport, independent of SWF frames and Dis64 file formats.

## Ownership and scheduling

`Idevector.wireparts` returns a new small segment array. For committed geometry,
segment0 is the current 48-byte style/history header and segment1 is the cached
immutable body, shared by reference. The document owner never concatenates the
large body for notified snapshots. Published arrays and strings must not be
mutated. Portable active drafts retain the existing contiguous fallback.
Geometry refinement and new committed-body serialization already run on the
geometry worker; importing a document can still prepare its first body on its
file/document owner. This is not zero-cost SVG parsing.

`Idedeskpane.startnotified` vector replies contain `parts` and a native ink fence;
legacy synchronous callers retain `drawing`. The desktop passes those immutable
parts to a bounded one-job mailbox. `idebulk.b` owns fragment slicing, envelope
encoding and output rendezvous. If an ink fence is present it assembles the exact
legacy fence value in that sender before sending data. No point arrays or mutable
document are sent on live pointer motion.

These are actual independently scheduled **Dis processes**, not a claim that each
is an OS thread. Native channel/GC access stays on the scheduler/heap owner.
Detached raster workers remain separate native jobs without managed pointers.
No background native thread reads Limbo strings or channels.

## Typed native extension

`Idehost.attachbulk(slot, output)` accepts a statically typed `chan of array of
byte` at slot1..8; nil detaches. It returns0 on success and-1 for invalid/duplicate
slots/channels or reuse of ordinary input/output. The sender's live stack roots
its channel from registration through explicit acknowledged detach. A native
borrow is not an owning GC reference. C accepts both native nil and the Dis nil
sentinel on detach; neither is dereferenced.

`editor_bulk.c` owns independent queue pairs and pump state. Each poll services
ordinary traffic first, then at most one copied geometry record per private port.
One blocked sender cannot fill a different broker. Only View/VectorData records
for that port's own pane are admitted; existing ticket/offset/extent/parser gates
still apply. The native owner consumes each record before the unblocked sender
runs and sends its scalar completion acknowledgement. Stop/detach drains the
single-flight transaction first. No managed heap address crosses into FlashVM.

## Atomic scene join

Small Begin/Item/VectorBegin metadata remains on the ordinary stream. `EndView`
with `first=declared vector-pane count` (1..8) opts into the private split-stream
join. End validates all scene metadata and forest ownership before waiting for
missing fragments. The last complete validated stream commits the deferred End
once; no partial shape is rasterized or used for hit testing. Legacy End.first0
still rejects incomplete geometry, preserving old decoder/refusal tests.

The desktop coordinator awaits scalar sender completions before beginning another
scene or acting on its hit geometry. This intentionally preserves the existing
whole-scene presentation barrier. Other Dis owners and brokers progress during
transfer; this series does **not** claim independently committed per-window scenes
or arbitrary input against a partially published scene. Ordinary typing/raster
scheduling retains its existing contracts. No frame-rate timer drives transfer.

## Reproduction

Build `disvm-editor` and all runtime modules together. Do not mix old/new
`idedeskpane`, `idevector`, `idehost` or `idebulk` interfaces.

```
ctest --test-dir build/workspace/disvm-editor -R 'bulk_|vector_control_cache|desktop_vector_protocol' --output-on-failure
```

`test_bulk_transport.py` opens two independently owned large SVG documents. In
both JIT/interpreter modes it requires overlapping actual transfers, exact
per-scene/per-port offsets and successful shutdown. `idebulk_test.b` checks real
native registration refusals and three repeated two-sender retirement rounds.
The decoder test checks deferred End, bad stream counts, stale tickets and
no partial publication. Vector tests compare segmented/legacy bytes, draft
compatibility and unchanged geometry serialization count across64 style changes.

`DVM_EDITOR_BULK_STATS=1` emits only frame/port/offset/size metadata, never drawing
content or pointers. These deterministic tests establish routing/ownership and
correctness; they do not measure physical-device latency or promise an FPS.
