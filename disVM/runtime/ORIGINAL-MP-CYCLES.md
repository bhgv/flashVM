# Sole-owner native Module and original-MP cycles

Copyright (c) 2026 bhgv

EN: Extend shared graph claims through original module data while synchronizing
native ownership with registry lookup, rollback and final reader epochs.
RU: Расширить shared-захват графа через исходные данные Module с синхронизацией
native-владения, поиска в реестре, отката и последних эпох читателей.

## Graph model and supported boundary

The [instance cycle pass](MODULE-CYCLE-RETIREMENT.md) captured only `Modlink.MP`.
A Module's separate `origmp` owner therefore kept some otherwise closed graphs
live. Discovery now attributes the actual `Module.origmp` cell to its sole owning
ordinary Modlink when the native Module reference count is exactly one. Both
managed edges are counted, including two references to the same MP allocation.
Distinct sole-owner Modules can participate in one managed cycle.

A sparse native Module vector follows each discovery node through bounded
scratch, Duke growth and published snapshots. Native metadata is independently
kept alive by the source Modlink until it is claimed; these vector entries do not
invent managed references. Original data uses the existing field guards,
expected-edge comparison, roots and discovery pins. A native owner acquired while
Duke is discovering forces retry instead of treating a temporary count observation
as permission to consume the original MP owner.

This attribution is deliberately limited to one native owner per Module. A cycle
requiring aggregation of multiple Modlinks that own the same Module remains
outside this new closure rule. Dynamic modules, library handles and native instance
data remain excluded. Stopped discovery retains its prior ownership algorithm.
No full concurrent shutdown or complete root-publication audit is claimed.

## Claim, registry and reader lifetime

[The native graph transaction](gc/fast/module_graph_claim.inc) prepares its vector
before taking the existing Module registry mutex. Inside that mutex it:

1. Verifies every selected native Module still has its sole owner.
2. Atomically claims all managed counts first. An independently owned managed
   link or new strong edge defeats this claim without changing native counts.
3. Claims the native Module counts from one to zero. A refused native claim rolls
   back every earlier native claim and all managed counts.
4. Creates the final reader cutoff. Failure restores both sets of counts.
5. On success, unlinks all claimed Modules before registry lookup can resume.

The mutex protects Module ownership/lookup, not VM execution. No allocation,
guest callback or destructor runs under it. The bounded callback takes world epoch
metadata after the registry mutex; root and field validation precede the registry
transaction. Native counts are never temporarily zero before managed claims, so a
valid direct retain through an independently owned link cannot see that transient
state. Existing external Module owners defeat the native claim.

Original MP fields, types, bytecode/JIT metadata and native allocations remain
intact through the final epoch. Only afterwards does the known graph finalizer
clear the consumed MP edges and call the loader's native finish path. It does not
release zero native counts or already-consumed original MP references a second
time. Imports, descriptors and code retain the reviewed cleanup outside the registry
mutex. The loader adaptation factors this finish from ordinary `unload`; pinned
upstream sources are unchanged.

A nonfinal `unload` now temporarily retains original MP before dropping its native
owner and publishes that extra managed owner afterwards. This supplies a new
cycle seed when removal of an outside Module owner makes a graph unreachable.
Module storage is never read after a nonfinal native release: only the independently
retained MP value remains in use. The temporary owner is published without an
intervening VM safepoint. Closed admission keeps the retained route.

## Cross-lane leaf wakeup

Repeated full instrumentation exposed a separate liveness defect. After the
outside Module owner disappeared, an original MP could retain one queued reference
and one root while Fast was idle. Cycles had converted that last candidate owner
into a leaf, but leaf publication notified Fast only for ACTIVE producers.

[Leaf publication](gc/fast/leaf_publication.inc) now notifies every ready-head
transition, including shared Cycles/Duke publication. Shared admission does not
imply that Fast is already running. This preserves event-driven progress without
parking other residents or replacing it with polling from a test.

The deterministic regression creates an actual mapped record with two owned
references, transfers one to the indexed queue and consumes only the private extra
owner. Cycles stays gated until Fast and its queues are observed idle. The real
Cycles-to-leaf publication must issue a real Fast scheduler request and physically
free the record before peer release. Reinstating the old ACTIVE-only condition
must produce the exact missing-notification predicate, not an accepted timeout.

## Actual runtime acceptance

The existing module-cycle fixture now covers 15 scenarios, each in C/JIT and both
poll implementations. Its original six scenarios remain, with native finalizer
observers extended to distinguish ordinary and graph-owned Module finish.

| Added scenario | Required real behavior |
| --- | --- |
| Same original MP | Account for both Module and Modlink references to one allocation |
| Cloned and original back edges | Claim native Module plus both independent MP allocations |
| Outside native lookup owner | A live scan preserves the graph; nonfinal unload supplies a fresh seed |
| Lookup after snapshot publication | Actual registry lookup defeats claim; later release permits collection |
| Native CAS refusal | Restore all managed counts after native reservation refuses unchanged |
| Final epoch refusal | Restore managed and native counts before snapshot replay |
| Two different Modules | Refusal at the second Module restores the first native claim as well |
| 2300-node original-MP ring | Resume Duke with native attribution preserved through growth |
| Cycles to idle Fast | Observe scheduler wakeup and actual physical leaf free |

The shared-data fixtures call real `linkmod(..., 0)` to create an initially empty
private instance, then attach an explicitly retained real original-MP owner before
publication. This does not change the loader to share MP automatically or claim
that `mkmp == 0` itself populates MP. Clone/back-edge cases use actual loader clones.
All fields, references, descriptors, allocator generations, registry entries and
native frees are real; no fake heap or substitute collector is used.

The refusal observers check exact restored counts before replay can consume any
reference. Final epoch observers keep two real admitted peers in their old epochs
for eight actual Fast responses. Physical target frees require zero roots, no
stopped world and at least two admitted peers. Metadata is withdrawn before its
actual free, and the Type creator must return to one after all target owners retire.
The changed-field test remains a guarded write by the Fast observer between
publication and confirmation, not a separate mutator thread racing a write.

Five semantic omission controls detect missing discovery, MP detachment, native
finish, final epoch waiting and the shared leaf wakeup. A native registry unit also
checks outside-owner refusal, callback rollback and batch unlink on both atomic
and forced-mutex count backends, alongside eight readers and four publishers.

## Validation and remaining scope

After the wakeup fix, ten full C-TSan repetitions completed all 60 positive
profiles (600 app runs). Another 100 repetitions of the formerly failing lookup
scenario completed all four C/JIT/poll profiles (400 app runs). There were no
sanitizer warnings in these 1000 runs. The native registry unit also passed under
TSan on both count backends. The focused selection passed 84/84 normal tests, and
ten longer GC checks passed 10/10. The final five-control acceptance was repeated
separately.

Execution is Linux x86-64. Generated JIT instructions are not C-TSan instrumented;
ARM64/RV64 runtime execution remains unverified. Multi-owner native Module closure,
unknown native finalizers, general stopped tracing, complete root auditing and
shutdown remain. Prior broader acceptance limits in [CONCURRENT-GC.md](CONCURRENT-GC.md)
and [FAST-FINALIZATION.md](FAST-FINALIZATION.md) still apply. This extends supported
shared closure; it does not establish complete no-STW GC or complete race freedom.
