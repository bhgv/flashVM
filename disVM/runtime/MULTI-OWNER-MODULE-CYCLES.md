# Aggregated native Module ownership in shared cycle collection

Copyright (c) 2026 bhgv

EN: Model one original-MP owner and all captured Modlink owners of each ordinary
Module, preserving data reachable from any live or outside native owner.
RU: Учитывать единственного владельца origmp и всех владельцев Modlink обычного
Module, сохраняя данные от любого живого или внешнего native-владельца.

## Ownership and reachability

The [original-MP pass](ORIGINAL-MP-CYCLES.md) required a Module native count of one.
Shared discovery now follows original data regardless of that count, so it can
find other instances that are reachable only through `origmp`. Each captured
ordinary Modlink contributes one native Module owner. A separate private hash maps
each Module identity to its first captured source; that source stores the single
physical `Module.origmp` edge. The original MP strong reference is counted once,
not once per instance.

[module_graph.c](gc/common/module_graph.c) aggregates the captured native owners.
If the actual native count is larger, an outside owner keeps original MP live.
A smaller count invalidates capture and requests retry. During reachability, every
live Modlink also keeps its Module's original MP live, even when a different,
dead Modlink supplied the stored physical edge. The first source is an indexing
choice, not lifetime authority for the native Module.

This distinction allows a dead instance cycle to retire while another instance
of the same Module stays independently rooted. A native Module is eligible for
whole-graph claim only if all its native owners are accounted for and all captured
instances are dead. Otherwise dead Modlinks use ordinary `unload`; original MP
ownership remains with the still-live Module.

## Snapshot and claim

Discovery and Duke growth preserve the Module hash and owner counts within the
existing scratch byte limit. New storage remains private to one collector. This
adds no global graph mutex, VM execution gate or world stop.

Published snapshots retain native identities only for wholly dead Modules. Original
MP fields of live or externally owned Modules are omitted from the consumed edges.
Their discovery pins are still released normally. Keeping such a native field in
the dead-edge release packet would incorrectly consume the still-live Module's
own reference; the acceptance suite explicitly detects this violation.

The registry transaction validates each unique Module's full expected count N.
Managed counts are claimed first, then native counts change from N to zero, and
the final reader cutoff is created. Refusal restores the original N, not one.
Lookup shares the existing registry mutex and cannot slip a new native owner
between validation and successful unlink. Allocation and finalization remain
outside that mutex. The existing one-owner registry API remains available.

A snapshot identifies the last graph-owned Modlink for each claimed Module.
Earlier dead links detach their native pointer without another reference decrement.
Only the final graph-owned link invokes native metadata finish. Managed link heaps
are physically released individually; the Module metadata and original MP edge
are consumed once. No metadata is freed while another graph-owned link still needs
it. Finalization follows the existing final reader epoch.

## Actual runtime acceptance

The module-cycle fixture now runs 27 positive scenarios with C/JIT and both poll
implementations. It preserves the previous tests and adds these cases:

| Added case | Required actual behavior |
| --- | --- |
| Two instances plus original back edge | Claim two native owners together and free metadata once |
| Aggregated native CAS refusal | Restore native count two and all managed counts |
| Aggregated final epoch refusal | Restore both ownership domains before replay |
| Outside lookup before discovery | Preserve the live cycle, then collect after native release |
| Lookup after publication | Reject the actual claim and later recover |
| 2300-node two-instance ring | Preserve native aggregation through resumed Duke growth |
| Outside original-MP ticket | Preserve reachable data until the ticket is released |
| One live member | Retire the dead member while keeping Module/original MP intact |
| Three dead instances | Atomically consume native count three and finalize once |
| Three-owner CAS refusal | Restore the exact original native count three |
| Two dead, one live | Retire the dead subset before the final member and metadata |
| Instance discovered through original MP only | Find native membership absent from the initial managed MP walk |

Construction uses the real loader, registry, mapped heap allocations, strong
references and native tickets. Allocation generation keys identify every target
physical free. Two permanent peers stay admitted and alternate real native epochs.
Each selected successful graph confirmation freezes their old epochs for eight
actual Fast responses before allowing any additional target frees. Mixed live
cases check both reclamation phases and retain the independent Type creator until
all target heap and native metadata frees complete.

The live-member checks wait until native handoffs and release packets have drained,
then verify that the remaining Module still owns exactly one original-MP reference.
They check the surviving fields and native count before releasing that member's
real rooted ticket. A native metadata free must occur after all required instance
retirement callbacks and registry unlink, with no stopped world.

Eight semantic omission controls require explicit status 12 and exact ownership
predicates. Three added controls restore the obsolete single-owner preflight,
omit propagation from a live Modlink to original MP, or incorrectly include a
live Module's original MP owner in dead-edge release. Compiler failure, signal,
process timeout and sanitizer warning never count as successful detection.

The native-only registry test additionally validates expected counts two and three,
outside-owner refusal, exact rollback and batch unlink, on atomic and forced-mutex
count backends. It retains the eight-reader/four-publisher membership stress.
Role-only tests reject any new aggregate claim before reading heap payload.

## Validation and remaining scope

Ten final full-runtime C ThreadSanitizer repetitions completed all 108 positive
profiles: 1080 application runs with no sanitizer warnings. The registry test also
passed under TSan on both count backends. The focused ownership/root/module/resident
selection passed 84/84 normal tests; ten longer GC checks passed 10/10. The final
eight-control acceptance ran three more times and passed.

Execution remains Linux x86-64. Generated JIT instructions are not C-TSan instrumented;
ARM64/RV64 runtime execution remains unverified. Ordinary native Module ownership
is now aggregated, but this does not complete no-STW GC. General stopped tracing,
unknown native finalizers, complete root and loader/code-cache auditing, bounded
snapshot overflow and shutdown remain. Earlier broader acceptance limits in
[CONCURRENT-GC.md](CONCURRENT-GC.md) and [FAST-FINALIZATION.md](FAST-FINALIZATION.md)
still apply.
