# Shared ordinary module-instance cycles

Copyright (c) 2026 bhgv

EN: Shared discovery follows an ordinary Modlink's MP owner and finalizes native
Module metadata only after graph claim and final reader cutoff.
RU: Shared-обход учитывает MP обычного Modlink и завершает native Module только
после захвата графа и последней эпохи читателей.

## Supported graph and ownership

The [ordinary module finalizer](MODULE-RETIREMENT.md) previously handled last-link
owners but excluded Modlinks from cycle discovery. A strong instance cycle such
as `Modlink -> cloned MP -> Modlink` could therefore require retained tracing.

[Candidate publication](gc/cycles/candidate_publication.inc) now recognizes the
same exact ordinary static Modlink identity as Fast. Dynamic code/data, native
instance data and library handles remain excluded. In
[graph capture](gc/common/graph_capture.c), an eligible Modlink contributes its
actual `MP` pointer cell. The existing field guards, expected-edge comparison,
outside-reference classification, queue roots and Duke discovery pins also cover
this edge. No new ownership count is invented for the native Module reference.
An unsupported Modlink remains live and prevents claiming anything it reaches.

A graph snapshot records whether it contains ordinary Modlinks. A successful
atomic claim consumes internal managed edges. After the final reader epoch,
[graph reclamation](gc/fast/graph_release.inc) sets a dead link's MP to `H` and
runs the known Module finalizer. It must not destroy that already-consumed MP
edge again. Native `unload` retains the existing registry unlink-last protocol;
its original MP owner enters shared RC before metadata disappears. Each other
Modlink or registry lookup keeps its own Module reference. Payload heaps and Type
owners are physically released exactly once by the existing graph path.

If RC acceptance is closed, shared Fast leaves module snapshots in their source
queue, including already claimed graphs. Stopped draining can finish them without
trying to publish `unload`'s managed owners into closed queues. This preserves the
existing shutdown boundary; it does not implement concurrent shutdown.

In this initial instance pass, native Module ownership was not a traced graph
node. The [original-MP follow-up](ORIGINAL-MP-CYCLES.md) now adds sole-owner
attribution, lookup-safe native claims and rollback. In particular,
`Module.origmp` is an independently owned managed reference. When that owner keeps
an instance cycle reachable, ordinary outside-count classification preserves it;
the initial change did not consume that native edge. The [multi-owner follow-up](MULTI-OWNER-MODULE-CYCLES.md) now extends native
closure; arbitrary native plugins and the whole loader/code-cache mutation surface
remain outside certification. `mkmp == 0` creates an empty MP instance; the later
fixture explicitly attaches a retained original-MP owner before publication.

## Real allocator and epoch acceptance

[test_module_cycle_retirement.py](test_module_cycle_retirement.py) relinks private
observers against the production allocator, loader/registry, collector and runtime
objects. A real Limbo app keeps two permanent residents admitted. They alternate
actual native epochs while at least one stays admitted, so target reclamation
cannot be explained by heap quiescence. The app runs with C/JIT and both poll
implementations for each of six scenarios:

| Scenario | Required evidence |
| --- | --- |
| One instance | Modlink, cloned MP, original MP and native Module physically freed |
| Registry lookup owner | Cycle heaps retire first; Module and original MP survive until lookup owner is released |
| External MP ticket | Actual Cycles live scan preserves both edges; later ticket release collects |
| Two instances | Two Modlink/MP pairs share one Module; both link heaps retire and native metadata is freed once |
| 2300-node instance ring | Bounded Cycles promotion, resumed Duke capture and every physical node free |
| Changed MP fields | A real guarded count-neutral field move makes actual confirmation refuse the stale snapshot; replay later collects |

For every target confirmation, observers freeze the peers' old epochs before the
real count claim creates its final cutoff. Eight actual Fast responses must occur
without any target heap or metadata free; only then may peers renew those epochs.
The allocator observers check generation identity, zero registry roots, no stopped
world and exactly-once completion after allocator return. Module metadata must be
unlinked before its actual native free. Independent Type creator ownership must
return to exactly one after all target frees.

The changed-field scenario moves the back edge between two genuinely mapped MP
slots under production write guards, without a retain/drop pair or synthetic
collector return. The Fast observer performs this write between snapshot
publication and confirmation while the snapshot owns the references. It tests a
real stale snapshot and replay, not a separate mutator thread racing a write.

Four omission controls must exit with status 12 and an exact semantic predicate:
remove module discovery, omit MP detachment, omit the native finalizer, or bypass
the final epoch wait. Build failure, process timeout, signal and sanitizer warning
never count as successful detection. All handwritten test/source files stay
within 500 lines; no upstream snapshot is changed.

## Validation and remaining work

Ten full-runtime C ThreadSanitizer repetitions passed all 24 positive profiles
(240 application runs), including actual stale-field rejection and 2300-node
rings. Generated JIT instructions are not C-TSan instrumented. Execution was Linux
x86-64; ARM64/RV64 remain unverified. The focused native/root/owner/module/resident
selection passed 84/84 tests. Ten additional longer GC checks passed 10/10.

General stopped tracing, unsupported
finalizers, complete root-publication auditing and shutdown still remain. Earlier
acceptance failures and limitations recorded in [CONCURRENT-GC.md](CONCURRENT-GC.md)
and [FAST-FINALIZATION.md](FAST-FINALIZATION.md) are not erased by these focused
passes. This is another supported shared graph class, not complete no-STW GC.

[Aggregated native ownership](MULTI-OWNER-MODULE-CYCLES.md) now follows all
ordinary instance owners, including original-MP-only membership and live subsets.
