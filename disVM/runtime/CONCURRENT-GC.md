# Concurrent GC: closed dispatch and emergency owners

The November 11, 2026 target is DisVM/Limbo execution and cycle collection without
stopping all mutators or reserving an idle heap against new entrants. FlashVM is
outside this series. The main runtime does **not yet** meet that target: it still
configures retained Fast/Cycles callbacks and can acquire stopped/quiescent world
ownership. Neither idle-only tracing nor disabling Cycles is final acceptance.

## Collector service

The paired `concurrent_fast` and `concurrent_cycles` callbacks in
[`DVM_GCWorkersConfig`](../include/disvm/gc_workers.h) select a closed route in
[`dvm_gc_worker_collect`](../host/gc_worker_pass.c). These callbacks run on bound
collector OS threads and supply their own object/root synchronization. They
return `DVM_GC_DONE`, `DVM_GC_MORE` for another bounded attempt, or `DVM_GC_FAILED`.
No result falls through to retained callbacks or a world stop. Existing shared
candidate/leaf callbacks are bypassed on these two lanes. Optional Duke keeps its
existing shared callback. An incomplete pair or missing `parallel_shared` is
rejected before any context factory runs. Retained run/root callbacks may be null.

The lifecycle/request implementation remains in
[`gc_workers.c`](../host/gc_workers.c); private state lives in
[`gc_worker_private.h`](../host/gc_worker_private.h). Host acceptance holds two
real mutators active without polling while both collector callbacks overlap them.
It checks bounded continuation, notifications during callbacks, idle wakeup,
error propagation, invalid configuration and joined lifetimes in both poll modes.
This is collector-service evidence, not a claim that a concurrent heap backend is
already complete. The main runtime has not been switched to these paired callbacks.

## Emergency-string retirement

[`dvm_runtime_exception_reserve_stop`](exception_reserve.c) transfers its existing
strong reference and root to a native FIFO, recording a reader cutoff before
publication. The world/allocator outlive all producer and collector joins.
[`dvm_runtime_exception_retire_shared`](gc/fast/exception_retirement.c) consumes
only on Fast, after the cutoff has elapsed. It validates the immutable Tstring
identity, removes exactly its own root, atomically drops its reference and frees
native string cache/pool storage only on the final reference. Another owner keeps
its own reference and root. No arbitrary managed destructor runs on this route.

The FIFO links are protected by their native mutex; only Fast consumes, so a peek
remains valid until that consumer takes it. Producers only append. Native mutexes
are not held during grace-period checks, root operations or storage release.
A missing world/epoch, diagnostic heap monitor or refused admission leaves the
entry available for the existing retained path. Thus this series removes the
normal emergency-string dependency on a stopped heap, without claiming removal
of every fallback. Shared Fast splits its work allowance between reserves and
other RC work; waiting for a reader cutoff requests another bounded attempt.
`NATIVEEXCEPTION` reports shared, overlapping-guest and retained releases plus
pending state after Fast has joined. Counts are process-lifetime observations.

The real-heap test
[`test_exception_retirement.py`](test_exception_retirement.py) relinks production
objects with native observation/coordination wrappers. Two residents alternate
reader epochs while at least one remains admitted. Actual `poolfree` observes the
final reserve release before those residents are allowed to leave. A second
reserve retains an independent external root/reference: Fast must consume only
its own occurrence. A registered native reader also holds the old epoch: the queue owners must
remain intact until its last access. C and x86-64 JIT run with both poll modes.
A private early-free control bypasses the epoch check and must fail at actual
`poolfree` while that reader is still registered. Disabling the shared
retirement call must fail with the expected semantic diagnostic, rather than a
compile error or process timeout. Imported Inferno sources remain unchanged.

## Remaining work

Convert managed-ticket and difficult-object retirement, remove retained graph
fallbacks from Cycles/Duke, replace stopped root snapshots, and audit allocator,
metadata, native callbacks and code/frame/module lifetime before switching the
main runtime. Confirm progress and cyclic reclamation under continuous allocation,
mutation, cancellation, memory pressure and reader churn. Port the complete guarded
JIT protocol to ARM64 and RV64; the current CLI still rejects guarded JIT on those
architectures. No ARM64/RV64 execution evidence is claimed by local x86-64 tests.

The build/test registrations are split by responsibility into
[`cmake/`](cmake). The split preserves the commands, properties and relative order
of all 346 pre-existing tests; concurrent emergency retirement adds one runtime
test. Every new/modified handwritten source or CMake module in this series is
at most 500 lines. See the [Russian implementation plan](MULTITHREADING-ROADMAP.ru.md).


## Validation of this series

Local Linux x86-64 / GCC 14 verification:

- All 19 standalone host tests passed. Closed concurrent dispatch also passed
  50 ordinary repetitions and 10 ThreadSanitizer repetitions.
- All 14 focused runtime tests passed, including actual exception retirement,
  root registry, resident error/root/native-call workloads, GC pending exchange,
  admission and managed/RC role controls. The strengthened exception test passed
  three further repetitions with held-reader and disabled-path/early-free controls.
- The old stack-check fixture did not declare internal `memchk` and failed to
  compile on GCC 14. Its exact declaration is now present; reentry acceptance passed.
- A complete 347-test pass found eight application timeouts plus that declaration
  failure. After the fix, a complete 339-test run excluding those eight timeouts
  passed 337 tests and exposed the two intermittent issues listed below. No claim
  of an entirely green broad suite is made.

| Existing issue | Comparison against original GC adapters |
| --- | --- |
| `idevector`, `idesyntax`, `replaceall`, `inklatest`, each C/JIT | Same 50/90-second timeouts reproduced with original reserve/GC/Fast translation units relinked into the same build |
| `disvm_sys_filesystem_interpreter`: finalizer quota reuse | 4 failures in 10 baseline runs; 4 in 10 modified runs |
| `disvm_shared_array_length`: capture replay PC | 3 failures in 10 baseline runs; 1 in 10 modified runs; the wrapper currently stores replay PCs in process-global static arrays |

These remain acceptance blockers for the broader multithreading effort. The
comparison preserves the build flags and other runtime objects; it is an adapter
comparison, not a claim of a separately rebuilt pristine checkout. The PC test's
state needs owner-aware isolation, and file quota/finalizer ordering needs a
separate runtime audit. Do not hide them by increasing timeouts or deleting checks.


## Follow-up: ticket handoff and readiness

The [shared managed-ticket pass](MANAGED-RETIREMENT.md) now preserves the old
reader cutoff and transfers RC ownership before dropping its ticket root/sidecar.
A refusal leaves its FIFO resources unchanged. The stopped ticket drain and
retained final-object destruction still remain available; this is not completion
of the no-STW target. Actual scheduler races exposed by full instrumentation were
fixed through [owner and ready publication](RUNQUEUE-PUBLICATION.md), without
introducing an execution gate or rebinding permanent resident registers.

The [physical Fast finalization follow-up](FAST-FINALIZATION.md) now verifies
actual final-string pool release, separate ticket/leaf reader cutoffs, recovery
from optional root OOM, and no stopped-admission attempt during the leaf wait.
It also records the controller fallback TLS race and the remaining acceptance
limits; the main runtime has not switched to the closed concurrent backend.

[Shared cycle-owner rescue](CYCLE-OWNER-RESCUE.md) extends publication to shared
native-ticket producers and sends pending mandatory owners to Cycles without
withdrawing their root. Actual small cycles and a 2300-node Duke ring are freed
before admitted peers leave. The main runtime still retains general stopped
tracing and unsupported finalizer/shutdown paths.

[Ordinary module retirement](MODULE-RETIREMENT.md) adds shared last-link MP
transfer, rollback and native Module/code cleanup through the existing registry.
Actual MP/metadata frees and executable-code unmap precede peer release. Dynamic
native modules, module cycles and closed-admission shutdown remain outside this
new path.

[Packet cycle retirement](PACKET-CYCLE-RETIREMENT.md) preserves field/snapshot
owners on indexed publication refusal and stages shared ticket owners until
their old root is withdrawn. Reachable cycles survive an actual scan before
their independent owner is released. General stopped tracing remains.

[Module-instance cycle retirement](MODULE-CYCLE-RETIREMENT.md) extends shared
discovery through ordinary Modlink MP fields. Actual graph claims and final
reader epochs precede native Module finalization; external MP and Module owners
retain their data. Native Module/original-MP cycle closure is still outside scope.

[Original-MP cycle closure](ORIGINAL-MP-CYCLES.md) now attributes original data to
sole-owner ordinary Modules, atomically claims native ownership with the managed
graph and unlinks before lookup resumes. Nonfinal native releases publish another
seed. A discovered Cycles-to-idle-Fast wakeup defect is fixed and has a deterministic
regression. Multi-owner native Module closure and general stopped tracing remain.

[Aggregated Module cycle ownership](MULTI-OWNER-MODULE-CYCLES.md) now counts all
captured ordinary Modlinks per Module, retains original data through any live
member and consumes native counts N-to-zero with exact rollback. Dead subsets
retire independently; original-MP ownership is released only for wholly dead Modules.
General stopped tracing, unsupported finalizers and shutdown still remain.

[Duke abandonment](DUKE-PIN-RETIREMENT.md) now preserves nonfinal candidate pins
in the mandatory queue. A real detached cycle retires after its original seed
was consumed while discovery still rooted it. Queue pressure preserves both
the root and journal cursor for shared retry. General stopped tracing remains.

[Duke allocation retry](DUKE-ALLOCATION-RETRY.md) distinguishes temporary native
snapshot allocation failure from the hard byte limit. Initial and growing
allocation refusal retain shared processing; supported candidates no longer
route to retained cleanup solely because snapshot allocation failed.

[Completed graph publication](DUKE-PUBLICATION-RETRY.md) now retries temporary
metadata refusal while Duke retains its complete discovery. Prepared native
buffers are freed and the leaf reservation cancelled before retry; no retirement
epoch is consumed for a failed preparation. General stopped tracing remains.

[Fast readiness retry](FAST-READINESS-RETRY.md) avoids a quiescent-only Fast
attempt while a guest is known active, including new work arriving after an
empty shared sample. Actual quiescent acquisition remains authoritative.
