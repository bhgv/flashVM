# Heap header observations — runtime0.70

A whole-header memmove reads Heap.ref nonatomically even if writers use atomic
primitives. The private snapshot helper now copies only bytes before/after that
field and obtains ref through the checked native observation primitive. Output
is private observer storage, not another Heap owner, root or graph clone.

Hosted preparation replaces13 whole-header reads:7 in string.c and6 in xec.c.
The string view additionally converts18 direct Heap count observations,2 Tstring
retains,4 static Type preflights and1 validated Type-ref observation. Snapshot-
local fields remain ordinary private values. Pinned source, native layout,
publication/cleanup order, Dis64 IDs and JIT emitters are unchanged.

## Storage and exclusion contract

Caller keeps a mapped live source header through ownership/protection and VM
exclusion. Geometry checks are not memory provenance or lifetime acquisition.
The destination must be private and disjoint. Nil/sentinel, alignment, overflow
and overlap refuse before dereference/output mutation. Real managed headers must
be native aligned; unaligned operand SLOTS still use their existing byte-safe
loads and are distinct from unaligned Heap storage.

Only ref is observed atomically. Color, Type pointer, profiling bytes and padding
are copied under existing exclusion/frozen-metadata requirements. The combined
header is not a globally atomic snapshot. No Type/Heap retain, GC root, mark,
allocation, callback or admission change occurs. Output fields are borrowed
observations whose use must end within their source lifetime/protection interval.
They cannot be destroyed or retained as if the copy owned referents. Registry
fallback requires caller not to hold the port registry mutex.

Source and destination address arithmetic is bounded before overlap checks;
private output is committed only after the complete temporary observation.
Count zero is not resurrected: helper can observe it in protected storage and
callers retain their existing invalid-source policy. Count==1 is not COW locking.

## Actual frozen-header diagnostic

The managed-owner diagnostic now includes heap_snapshot_probe.c. A real String
source owner remains in ACTIVE admission while8 native count-only workers run
16000 balanced generic native counter pairs. They receive only borrowed count
storage; they do not read/write payload/header fields or call the VM. Source
keeper and immutable non-count fields survive through all joins. Controller
makes4000 snapshots; observed count stays1..9 and immutable Type remains correct.
After joins count must be1, then source destruction/Fast retirement happens with
no later source read. This test is not general permission for native Heap access,
parallel mutators, concurrent metadata copying or moving-GC pinning.

Instrumented C TSan exercises the observer against real concurrent count writes.
Atomic-selected and forced-registry count paths are separate executions. The
GNU-linker/Makefiles driver injects wrong count, wrong Type observation and false
overlap success only during the real probe. Exact semantic status10 must precede
unsafe access. These are result/refusal controls, not a claim that they remove
atomic instructions or establish whole-VM race freedom. Compiler/signal/timeout/
sanitizer failures do not count as detected semantic controls.

```sh
export TMPDIR="${TMPDIR:-$PWD/build/tmp}"
mkdir -p "$TMPDIR"
python3 disVM/runtime/test_heap_snapshot.py \
    build/workspace1735/disvm-editor/disvm \
    build/workspace1735/disvm-editor/bootstrap/limbo-bootstrap \
    disVM/upstream/module
```

The shared private relink utility leaves production objects unchanged. Matching
native console Makefiles metadata and private runtime headers are required; the
source SDK is not a new public snapshot/pointer capability. Console/editor owner
drivers require this probe marker. Generated JIT instructions are not TSan-
instrumented; local Linux x64 checks do not certify target ARM64/RV64 execution.

Other C/JIT/GC accesses, metadata/payload writes, source slots, allocator and
graph/root/collector protocol still require serialized managed admission.
[Current status](HEAP-PARALLELISM-STATUS.ru.md).
