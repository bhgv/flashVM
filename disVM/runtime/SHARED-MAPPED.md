# Guarded MOVMP replacement — runtime0.91

`--gc-worker --guarded-copies` now includes `MOVMP` in addition to the five
scalar/raw copies. Default startup retains the original mapped-copy path. The
hosted x64 aggregate tail already enters process-live C, so the selected C
transaction is also the JIT implementation. Other guarded JIT backends still
refuse the profile; their explicit C path needs independent target validation.
Runtime0.92 also converts `HEADMP`: [SHARED-HEAD.md](SHARED-HEAD.md). Mapped list
construction, channel movers and other writers remain outside this transaction. NativeWorld still serializes general managed code.

## Transaction and ownership

The opcode preserves its existing Type/index, empty-record, nil and span checks.
The adapter hashes the complete reviewed final MOVMP body before inserting one
opt-in branch. `shared_mapped.c` implements the replacement separately from opcode
generation, generic byte transfer and native root-group bookkeeping.

1. Under existing ACTIVE admission, capture validated immutable map offsets.
   Prepare a complete incoming byte snapshot, per-occurrence metadata, a root-key
   vector and a root-group reservation. All five native allocations finish before
   guards or ownership effects. Failure releases private metadata and raises
   outside locks with source owners and destination unchanged. No map references
   use the existing raw-copy helper without this preparation.
2. Claim the complete source/destination range union through the same domain as
   scalar and pointer copies. Capture incoming bytes and displaced references
   only after successful acquisition. Check all mapped referents before retains.
   Geometry/count checks do not prove mapping, provenance or independent lifetime:
   those remain caller/Type/source-container obligations under existing admission.
3. Acquire one strong count for every incoming non-H occurrence and apply the
   existing checked marking barrier. On refusal, undo only successful new retains
   while the original source is protected. These drops cannot retire an object.
   Conservative color/dirty effects already performed are not rolled back.
4. Publish all displaced owners as a preallocated root group, then publish the
   incoming snapshot once. Overlap/self-alias reads use the complete snapshot.
   The destination now owns new counts; the native retirement vector owns the
   old destination counts. No destructor/error/VM callback spans range guards.
5. Unlock. In historical `freeptrs` map-byte/reverse-bit order, withdraw just the
   current root occurrence, then invoke its consuming destructor. Other old
   occurrences stay visible to actual GC during callbacks. An emptied group
   remains active until final clear. Clear roots, destroy native metadata, finish.

`root_batch_remove` addresses an occurrence by its original vector index. It
rejects invalid/already withdrawn slots and never consumes another group or an
ordinary registration. Clear now skips withdrawn slots, removing every remaining
node atomically to locked visitors. Publication/withdrawal/clear do not allocate;
the registry mutex may wait. Each batch still requires one exclusive native user.

Each consuming destructor has an actual `kprotect` boundary. If it raises, the
transaction does not retry that owner or undo committed destination bytes. It
continues retiring remaining owners, clears its own roots/metadata, restores the
first error text and rethrows. This does not repair arbitrary partial failure
inside a destructor, allocator or native callback; their existing consuming/error
contracts remain required. The positive error diagnostic throws AFTER consuming
its current owner, exercising safe continuation rather than inventing rollback
of an unknown destructor. Callbacks may change REG; the transaction never rereads
REG operands or the Type after publication.

Fast may independently root a deferred last owner after the transaction withdraws
its occurrence. A nonzero root count after `destroy` therefore does not by itself
mean the batch leaked. Final Fast/Type baselines, exact withdrawal before each
consume and remaining-root visibility distinguish the two owners.

## Validation and scope

The actual hosted diagnostic uses genuine dtype/String allocations and unaligned
native record views borrowing independently rooted source owners. It covers all
five preparation allocation refusals, invalid mark rollback after multiple retains,
publication before callbacks, real callback-driven Cycles, consumed-owner error
draining, self alias, duplicate incoming/outgoing occurrences, nil replacement,
boundary canary and final real Fast/Type baseline. C/JIT and both polling backends
run every case. Six actual-boundary controls omit claims, rollback, root publication,
withdrawal, unlock or data publication; each must stop at semantic exit 10 before
unsafe GC or retirement. Signals, compiler failures and timeouts never count.

A real guest reaches adapted MOVMP through both dispatch modes. The existing
aggregate-tail suite also runs the guarded profile: actual tail coverage, each
cache unmap, invalid-Type errors, combined error/unmap, NULL/foreign/held-guard
preflight and bad-CALL/old-emitter controls. Held guards cause the guarded MOVMP
tail to decline BEFORE reading operands. Other aggregate tails/default paths
retain their prior behavior.

Native root tests cover exact indexed withdrawal/repeated refusal/sparse clear,
ordinary/group separation, eight OS owners, 16000 cycles and 1000 full-group
visitor scans on both publication backends. Five actual source changes per
backend require ten semantic controls. Hosted diagnostics use actual managed
headers; opaque native-key tests run separate processes without a collector.
No fake Heap or Module root is used. Emitted machine bytes are uninstrumented.

The prepared native scratch is temporary, not a permanent per-thread heap or an
object quota. It adds allocation/locking cost to mapped copies, so the profile
remains opt-in. No speedup, local/shared compiler classification, concurrent
managed execution, physical ARM64/RV64 acceptance or new GC publication epoch
is inferred. Other reference writers, source/list/container lifetime, scheduler/
channels, root/graph/GC epochs and code/module/frame lifetime remain open.

No Dis64 file layout, encoding/ID, ordinary WORD32 LEA, shared file-call admission
or named Inferno launch/cache agreement changes follow from this memory profile.
The retained engine/bootstrap snapshot remains independent of current Inferno.

21 affected console/editor CTests pass. Native withdrawal tests pass TSan and
O3 nonrecovering UBSan, including ten source controls. Actual mapped positives
and guarded aggregate-tail positives pass TSan; emitted JIT bytes remain
uninstrumented. Checkout/installed examples, installed native suites and retained
1096 engine/312 bootstrap file pins verify.

Local cost after regression runs: same binary/image, 20000 mixed String-record
assignments, startup included, warmup plus median of three alternating samples.
C default/guarded 0.140s/0.169s (~20% extra); JIT 0.105s/0.142s (~35% extra).
Not isolated-op, whole-IDE, target or general statistical evidence. The guarded
profile includes scalar/raw protection as well; this is not a measurement of one
isolated MOVMP. Preparation currently allocates five native blocks per mapped
replacement. Reuse/local classification remain future optimization work.

## Reproduce

```sh
python3 disVM/runtime/test_shared_mapped.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/test_aggregate_tail.py RUNTIME LIMBO INCLUDE_DIR --guarded-copies
python3 disVM/runtime/test_root_batch.py
python3 disVM/runtime/benchmark_shared_mapped.py RUNTIME LIMBO INCLUDE_DIR
```

Matching console CMake/GNU-linker objects are required for private hosted relinks.
Positive instrumented runs accept `--positive-only`. Native SDK tests are under
`source/threading/`, with their complete private header/port closure installed.
See [root groups](ROOT-BATCH.md), [byte copies](SHARED-BYTES.md),
[aggregate tails](AGGREGATE-TAIL.md) and [threading](THREADING.md).

Measured costs and pending investigations: [PERFORMANCE-COSTS.md](PERFORMANCE-COSTS.md).

## Shared admission for array assignment

`--parallel-slicela` enables an experimental path with `--guarded-copies`
and multiple managed workers. It accepts mapped and raw `SLICELA` between arrays or views captured from typed frame
slots, with an immediate or private frame index. A pinned view retains the
backing array through its root link. Raw payloads use the same range guard
and overlap-safe byte copy, with no displaced-reference bookkeeping.

The runtime retains and roots both arrays and reserves copy metadata under
exclusive admission. Once admitted in `SHARED`, it rechecks both array
descriptors, acquires the union of the payload guards, snapshots the source,
retains the incoming references and roots displaced references before
publishing the destination. It releases the guards and returns to exclusive
admission before retiring displaced references and releasing the array pins.
Contention can replay the original instruction before publication. The cycle
collector acquires stopped-world admission between shared intervals.

The hosted GC test exercises 64 publications of two mapped cells across
two guest writers, validates the surviving references after unwind and
collection, and checks overlapping shared view assignment after dropping
the direct backing-array reference. The `parallel_slicela_commits` diagnostic distinguishes those shared
publications from other native suspensions.

`--parallel-movmp` admits mapped `MOVMP` copies between two complete, aligned
records in the current live frame. Both record maps must match the frame
map across the entire byte span. The suspended process retains the frame
and its type. Publication and retirement use the same range transaction
as `SLICELA`. Other addressing modes and buffered channel transfers retain
exclusive admission. The GC guest checks 32 mapped frame copies with an
owning reference and a scalar field in each record.
