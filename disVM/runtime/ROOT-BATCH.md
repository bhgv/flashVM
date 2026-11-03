# Preallocated root groups — runtime0.90

Root batches reserve native metadata before entering an access interval. Publication
and clear use the actual external-root registry mutex and never allocate. The
existing stopped-world collector sees either the complete group or none of it.
This is a prerequisite for retaining displaced aggregate references across
destructor callbacks. Runtime0.91 uses this API for opt-in MOVMP; runtime0.92 also roots the HEADMP
source list. See [mapped copies](SHARED-MAPPED.md) and [list heads](SHARED-HEAD.md).

`root_batch.c` contains the group algorithm, `root_registry.c` owns the shared
ordinary-root lists, and `root_registry_private.h` describes their private native
bridge. No managed Heap header, pointer encoding, Dis64 ID or public file layout
changes. General managed execution still requires NativeWorld exclusion.

## Contract

1. Create a nonzero capacity reservation outside object/access locks. Registry
   initialization and the two private allocations can fail. No roots are added.
2. While independently owning and protecting all referents, publish a vector of
   nonnull keys, at most capacity. Duplicate keys are separate root occurrences.
   In hosted GC the keys must be genuine independently valid managed Heap headers.
   No managed pointer is retained, validated, marked or dereferenced by this API.
3. Withdraw each exact occurrence immediately before its consuming destructor,
   keeping all remaining owners rooted, or clear the entire group before releasing
   its independently protected owners.
   It removes all occurrences atomically to locked visitors, without callbacks or
   allocation. Ordinary `ptrdel`/root-remove cannot steal batch occurrences.
4. Destroy only an inactive, externally quiescent reservation. Active destruction
   refuses unchanged, including an active empty group. Reservations are reusable.

Each batch has one externally exclusive native owner at a time. It may not race
its own publish/clear/destroy. Different batches may publish concurrently. The
registry and its mutex persist for process lifetime. Visitor callbacks remain
bounded, nonallocating and nonreentrant; they must never wait for an object guard.
Mutex acquisition may wait: allocation-free does not mean lock-free or wait-free.

Invalid vectors are checked completely before any list mutation. An empty vector
is a valid active publication. Ordinary count/visit includes both kinds of roots;
ordinary removal searches only ordinary nodes, and batch clear unlinks exact node
identities. No batch node is placed in the ordinary reusable pool. Clear proceeds
in reverse insertion order to avoid quadratic traversal of the batch's own list.
Bucket collisions and competing roots can still cost traversal; no general latency
bound is claimed.

This API alone is not a GC publication barrier or permission to acquire root locks
under arbitrary object locks. Hosted use currently requires the existing ACTIVE/
stopped-world exclusion and an agreed lock order. Concurrent graph/root epochs,
aggregate source validation, retains/rollback, destination publication and callback
retirement remain separate work. It is unsafe to replace ownership with a registry
entry or to publish arbitrary pointers for the collector to inspect.

## Validation

Native metadata tests use ordinary C addresses in processes without a collector.
Eight OS owners race cold registry initialization and perform 16000 complete
publish/clear cycles while 1000 locked traversals require whole groups. Tests
cover duplicates, ordinary/batch separation, two allocation refusals, empty active
groups, invalid vectors, capacity overflow, reuse and complete joined teardown.
Allocator interception checks attempts, including warmed metadata. Four actual
source mutations on both publication backends must fail at semantic exit 10,
before unsafe traversal/reuse: missing publish, missing clear, stealing ordinary
remove and allocating during publication. The existing registry controls remain
independent.

The managed test uses two real independently owned Strings and actual cell guards.
It reserves before guards, publishes duplicate groups without allocation, removes
one group, unlocks, leaves ACTIVE admission and performs 32 genuine stopped-world
Cycles passes on native collectors. Both Strings remain intact at strong count 1;
the remaining roots are cleared before destruction, final Fast restores the Type
baseline. C/JIT and both polling backends execute the test. Two controls omit
publication/clear and must fail before any collector or object retirement.
Root batch metadata is not a new strong owner or a parallel managed mutation grant.

18 affected console/editor CTests pass. Native positives pass TSan; native
controls pass O3 nonrecovering UBSan. Actual managed positives pass TSan.
Installed suites/examples and retained engine/bootstrap pins verify. No emitted
JIT instrumentation or physical ARM64/RV64 acceptance is inferred.

## Reproduce

```sh
python3 disVM/runtime/test_root_batch.py
python3 disVM/runtime/test_root_registry.py
python3 disVM/runtime/test_managed_root_batch.py RUNTIME LIMBO INCLUDE_DIR
```

Native and instrumented-positive tests accept `--positive-only` where applicable.
For an installed SDK, native tests are under `source/threading/`; its private
header/source closure and `source/ports/` are included. Managed diagnostic relinks
need matching console CMake/GNU-linker objects. Set TMPDIR when /tmp is unavailable.

See [root registry contract](root_registry.h), [shared copies](SHARED-BYTES.md),
[threading status](THREADING.md), and the native example below. This snippet uses
opaque native addresses only in a process without a managed collector:

```c
#include "root_batch.h"

/* Own both native keys until the group is cleared; no managed GC in this process. */
int main(void)
{
    int first = 1;
    int second = 2;
    void *keys[2] = {&first, &second};
    DVM_RootBatch *batch = dvm_runtime_root_batch_create(2);
    if (!batch)
    {
        return 1;
    }
    if (!dvm_runtime_root_batch_publish(batch, keys, 2))
    {
        (void)dvm_runtime_root_batch_destroy(&batch);
        return 2;
    }
    /* Use only bounded work here. Registration grants no ownership of keys. */
    if (!dvm_runtime_root_batch_clear(batch))
    {
        return 3;
    }
    return dvm_runtime_root_batch_destroy(&batch) ? 0 : 4;
}
```

Compile with `root_batch.c`, `root_registry.c`, Linux `channel_sync.c`, their
runtime/ports include paths and `-std=c99 -pthread`. No module or heap is fabricated.
