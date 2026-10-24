# Separate Inferno/Dis versus FlashVM GC comparison

[Русский](GC-COMPARISON.ru.md) · [Reproduction](../tools/gc-compare/README.md) ·
[Fast/Cycles workers](../disVM/host/GC.md)

## Decision

**Overall Inferno GC superiority has not been demonstrated. Do not replace the
FlashVM collector or designate an extracted common GC yet.** Inferno offers
immediate RC reclamation and incremental tracing; FlashVM already integrates
its dynamic objects, AS roots, strings, resource cleanup and resurrection.
Replacing collector functions alone does not preserve those host contracts.

This is a source/correctness comparison and a real FlashVM baseline. Inferno's
explicit test-pool timings are not used as production allocator/VM performance.
There is no claimed numerical speed or memory ranking between equivalent ready VMs.

## Pinned sources and method

FlashVM GC/string-GC match the published v0.1535 tree (`930eaa97`), with no algorithm
changes during measurement. The native library is Release. Inferno experiment is
pinned at `3e3536578b969cf1a460210d8ec667713869ccfc`; a remote check found that same
commit. The host is Linux AMD64, GCC 13.3.0/glibc 2.39. Five fresh processes per
case, 10,000 and 50,000 elements, explicit full collections with auto scheduling
disabled. Windows/macOS/Android are not represented by these numbers.

## Mechanisms

| Criterion | Inferno/Dis | FlashVM |
| --- | --- | --- |
| Core | RC destroy plus colour-based rungc | Precise object tracing/mark-sweep |
| Acyclic garbage | May release immediately at last reference | Releases at collection |
| Cycles | Trace from exact roots | Trace from exact roots |
| Scheduling | Adaptive scan quanta | gc_step threshold triggers full gc_collect |
| Hard pause bound | None: roots/arrays/recursive RC can take longer | None: full trace/finalization/sweep |
| Roots/edges | Type/frame maps and container metadata | Dynamic properties, AS registers/stacks/captures, pins/native roots/resources |
| Strings | Managed Dis objects | Arena by default; explicit traced mode exists |
| Finalizers | Type/native-resource destructors | Script/native finalizers, resurrection/remark |
| Allocation failure | Needs full host/allocator acceptance | Dedicated tested OOM regressions |
| Shared-heap parallel safety | Existing global GC/RC is not automatically thread-safe | Context owner-thread collector |
| Compaction | Pool callback mechanism; no default move callback | No automatic moving/compacting GC |

Inferno is a useful source for earlier RC release and incremental phases. Those
traits do not prove lower total CPU/RSS after adaptation to existing FVM_Object
layouts and dynamic containers.

## Correctness evidence

Sixteen real FlashVM regressions pass: cycles, deep graphs, native roots, nested
scopes, resurrection, release-resurrection, strings/transport, finalizer snapshots
and three allocation-failure scenarios. This is not full VM acceptance.

Actual Inferno destroy/rungc bodies pass cascading RC, dead cycles, global/frame
root retention/removal and an unmapped address-valued scalar test. The pool is an
explicit deterministic fixture, not the production allocator. This establishes
algorithm behavior, not arena quality, peak RSS or complete Dis execution speed.

## FlashVM baseline

Median CPU time per full collection of 50,000 elements, five fresh runs:

| Case | Collection CPU | Result |
| --- | ---: | --- |
| Independent unreachable objects | 5.863 ms | All 50,000 reclaimed |
| Unreachable ring | 7.503 ms | All 50,000 reclaimed |
| Pinned ring | 5.381 ms | Preserved |
| Same ring after unpin | 4.534 ms | All 50,000 reclaimed |
| Unreachable arena strings | 0.006 ms | 888,890 buffer bytes remain until context destruction |
| Same strings, traced mode | 6.337 ms | 0 of those buffer bytes remain |

CPU time is not wall-clock latency or p95/p99. This small baseline is environment
sensitive, not a product performance guarantee. Whole-process peak RSS includes
runtime, allocator and test overhead, not just live managed bytes. Raw evidence
is delivered separately from source. The cheap arena collection did not free
strings; it cannot be interpreted as a superior GC result. Traced mode already
exists, but changing its default requires borrowed-string compatibility checks.

## Inferno allocator gate

The retained 32-byte pool quantum conflicts with native LP64 metadata: the leader
Btail at offset 24 overlaps csize at offset 24, and minimum free blocks cannot fit
the native header. Performance with that layout is not a valid GC comparison.
A separate owner-repository patch derives a sufficient quantum from Bhdr/Btail,
preserves ILP32 and passes ILP32/LP64/LLP64 layout models under UBSan. The old source
fails as a negative control. Auditing the fixed `ae99aac` gives PASS_LAYOUT_ONLY;
that is not full allocator/64-bit VM acceptance.

## Replacement gate

Compare equivalent graph/root lifetimes, real allocators, finalizers and string
policies; measure total CPU, wall-time pause distributions, peak/retained bytes,
fragmentation and arena release across target platforms. Require the same
correctness/OOM/resurrection checks for both backends. If the advantage is then
shown, extract a common `gc/` engine with Dis/FlashVM root, edge and finalizer
adapters. Keep native layouts isolated and the allocator independently selectable.
No FlashVM GC replacement or selected common-GC extraction is made here.

Sources: [Inferno gc.c](../disVM/upstream/libinterp/gc.c),
[Inferno heap.c](../disVM/upstream/libinterp/heap.c),
[FlashVM gc.cpp](../flashVM/src/core/gc.cpp),
[FlashVM string_gc.cpp](../flashVM/src/core/string_gc.cpp),
[public policy/stats](../flashVM/include/flashvm/gc.h).
