# Live native arena statistics

Copyright (c) 2026 bhgv

[Русский](MEMORY-STATISTICS.ru.md) · [Runnable example](../examples/memory-statistics/README.md)

Processctl.memorysnapshot now returns one coherent accounting record for the
hosted VM's managed arena. It is safe while owners allocate/free concurrently:
the reader takes the allocator's existing metadata mutex and copies O(1) counters.
It does not walk live objects, inspect Type/REG/stack, take managed references or
stop other owners. Mutations already hold that same pool mutex; updates add no
new lock or callback. Snapshots are observations, not memory access capabilities.

## Scope and units

| Field | Meaning |
| --- | --- |
| arena | Whole calloc arena extent, not physical committed/resident bytes or process RSS |
| header | Arena header outside the block partition |
| live / free | Allocated/free block spans, including native overhead and alignment slack |
| liveblocks / freeblocks | Allocated blocks/free extents, including unpublished construction buffers |
| overhead | Per-block native header, allocation generation and tail bytes, excluding alignment slack |
| allocations / releases | Successful pool allocations/frees since this pool's initialization |
| peak | Greatest simultaneously allocated block span |
| flags | Allocation/release history increments clamped at signed-big max |

The exact partition is `arena = header + live + free`. Current gauges are bounded
by the arena size and never saturate. Without flags, `allocations - releases =
liveblocks`. History saturates at 9223372036854775807: a flag means at least one
further increment was clamped. Peak is an exact current-lifetime high-water mark.
Refused allocations/frees and refused destruction do not change these counters.
Pool destruction requires joined users/no live blocks and resets accounting.

`livecapacity = live - liveblocks*overhead`, likewise for free. Capacity still
includes alignment slack and managed Heap headers. It is not requested payload,
reachable application data, per-type bytes or a count of published variables.
This allocator does not coalesce free blocks. Aggregate free capacity is not the
largest free extent and does not guarantee one equally large allocation succeeds.

Native malloc metadata outside the pool, code/Type/module storage allocated
elsewhere, OS stacks, files/sockets, diagnostic buffers, origin/retainer attribution
and process RSS are excluded. Logical release may precede deferred RC/GC reclamation;
free counters change only when poolfree actually releases the block. No live-object
catalog, allocation-origin IDs or per-thread/per-type attribution is claimed.

## API and publication

`Processctl.memorysnapshot()` returns:

```
(status, arena, header, live, free, liveblocks, freeblocks,
 overhead, allocations, releases, peak, flags)
```

Status1 means a coherent record. Status-1 means unavailable/inconsistent native
accounting; every payload field is zero. The native function also zeros output
on invalid/uninitialized state. Caller must keep pool/mutex alive through return;
a mutex cannot protect its own lifetime against concurrent pool destruction.

The C boundary stages one native record, then publishes a pure-scalar 96-byte
NC64 result through the existing shared range-guard domain. The scalar adapter's
limit is extended from64 to96 bytes; there is no raw store or split publication.
Busy retries only publication, never resampling counters. A new tuple/field read
is a different instant: do not combine separate snapshots into one partition.
No public Dis64/ISA/Sys format ID or execution-mode change is introduced.

`module/memoryview.m` and `debug/memoryview.b` expose
`inspect() -> (status, ref Stats)` with named fields and derived capacities.
All data belongs to the caller; the library has no mutable globals. Allocation of
the returned ADT happens after the accounting instant and is not included in that
same sample. Managed allocation errors propagate; there is no retained ticket or
native request to clean up. Native refusal produces nil, never a partial view.

## Verification

A private native unit uses eight actual OS threads with3200 allocation/write/size/
free rounds total. An independent oracle sums actual native block headers under
the pool lock and compares cached counters at the same instant. It also checks
whole-block allocation/no free remainder, unchanged refused allocation/destruction,
zero failed output, signed-big history saturation and joined quiescent destruction.
This oracle never reads a managed object or uses the counters to compute its sums.

The VM fixture runs four allocating guests and a continuous shared-result observer.
Two hundred snapshots check exact partition/history and monotonic traffic; ordinary
payloads and child joins must finish. Adjacent canaries protect the96-byte tuple.
The Limbo facade, public example and explicit unavailable/zero-output path run
C/JIT with fast/locked polling; the main fixture additionally runs quantum1.
Two copied-output controls corrupt free-span or allocation-history projections;
exact DETECTED, normal completion and full native/GC drain are required. They are
projection controls, not claims of injected allocator-accounting update omissions.

Final selected regression passes14/14 ordinary and14/14 ThreadSanitizer groups,
including diagnostics, cancellation, retirement, table ownership, packet charging
and fast claims. Existing actual-pool16000-allocation test and its three controls
also pass. Mandatory architecture list now contains60 workloads (list verified);
full new architecture execution remains a separate CI result.
