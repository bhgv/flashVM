# Atomic collector work consumption — runtime0.86

The hosted collector now consumes `nprop` with one scalar exchange to zero,
instead of reading it and clearing it later. Under the existing stopped-world
admission this preserves collector behavior. As preparation for concurrent
publication, it removes the read/clear window in which a later notification
could be overwritten. This does not authorize concurrent collector epochs.

## Operation and ownership

`dvm_gc_field_exchange(destination, value, previous)` replaces one independently
live aligned native int and writes its predecessor into private disjoint output.
NULL, unaligned and same-cell arguments refuse before access. Competing accesses
must use the scalar API. Source storage lifetime is supplied by the caller;
alignment checks are not mapped-pointer validation or root acquisition.

Native lock-free int targets use acquire/release exchange. The portable fallback
holds the existing port registry mutex across read and replacement. No allocator,
callback, VM error or managed admission occurs while that mutex is held. Callers
must not already hold it. The hosted wrapper returns the predecessor and treats
malformed geometry as a fatal host invariant after releasing native locks.

A notification preceding the exchange is included in the returned value; a
notification following it remains pending. Repeated notifications can coalesce:
this is a sticky work flag, not an event counter. Color publication still occurs
before flag publication and is not a single atomic multi-field operation.
Selector changes, root/edge publication, color traversal, epoch completion,
`gchalt`, cursors and collector state still require their existing exclusion.
In particular, observing zero is not by itself proof that an epoch may finish
while mutators run. NativeWorld remains serial for general managed execution.

## Exact hosted integration

`prepare_gc_fields.py` keeps its reviewed complete pre-adaptation source hashes
and native-header hash. It replaces the actual end-of-scan pending check with
exchange-to-zero and removes the later reset. The zero branch still advances
color/root-set state under stopped-world admission. The nonzero branch retains
another scan's work notification. New marks after this consumption are not
cleared by a subsequent store in this collector invocation.

There is no vendor-snapshot edit, Heap/Type layout change, shared Dis64 ID,
serialized native pointer or new file FRAME/CALL/RET/LEA admission. Existing
unchecked generated accesses remain protected by serialized admission.

## Tests and limits

`test_gc_exchange.py` compiles the actual helper and native ports for both native
and forced-lock backends. Eight joined OS writers perform16000 unique scalar
replacements from a synchronized start gate. After joins, the complete returned
predecessor history must contain every token exactly once, preserve each writer's
program order and reach the initial zero. Serial checks cover refusal, returned
predecessor, replacement, flag consumption, later publication and hosted wrapper.
Four actual-source controls omit replacement or return the wrong predecessor;
all require semantic10 before thread launch. This is a single-cell history test,
not concurrent managed-graph or GC-epoch acceptance.

`test_gc_pending.py` privately relinks matching real runtime objects, intercepting
actual collector exchange and zero stores. Its real rooted String survives32
Cycles then retires through Fast. C/JIT and both poll modes must reach the new
collector operation. Restoring the old split check/reset fails semantic10 BEFORE
its reset write. Removing consumption separately fails coverage after the actual
worker wait/controller reentry. Production objects remain unchanged; the added
private collector replacement uses the same source-view headers and flags.

Native and forced-lock positives pass TSan; both backends and four source controls
pass nonrecovering UBSan. Actual hosted collector positives pass TSan with C/JIT
and both polls; emitted JIT bytes are uninstrumented.44 selected console/editor
regressions pass. This evidence does not prove shared payload guards, parallel
root traversal, concurrent epoch transition or another physical target.

Installed SDK includes helper, native ports, tests and this contract. Run
`python3 source/threading/test_gc_exchange.py` from an installed package. Actual
collector private relinks require matching GNU Makefiles runtime build metadata.
Set TMPDIR to a writable directory if /tmp is unavailable. Instrumented positive
runs accept `--positive-only` to omit strict source controls.

See [scalar GC fields](GC-FIELDS.md), [threading](THREADING.md) and
[checked aggregate entries](AGGREGATE-TAIL.md).
