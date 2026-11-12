# Private RC source composition

`runtime_rc.c` composes the following private fragments in dependency order.
They remain one C translation unit: callbacks, queue state and static helper
visibility are unchanged. No public ABI or new global execution mutex is added.
The initial decomposition preserved the complete ordered original bytes before
the functional changes; subsequent formatting preserved non-brace tokens and
preprocessor directives. Compilation and runtime tests remain necessary evidence.

| Responsibility | Fragments |
| --- | --- |
| Session state and joined lifecycle | [rc_state.inc](rc_state.inc), [rc_lifecycle.inc](rc_lifecycle.inc) |
| Leaf and mandatory owner publication | [leaf_publication.inc](../fast/leaf_publication.inc), [owner_publication.inc](../fast/owner_publication.inc) |
| Mandatory FIFO rescue | [owner_drain.inc](../fast/owner_drain.inc) |
| Frame and child owner packets | [frame_publication.inc](../fast/frame_publication.inc), [packet_children.inc](../fast/packet_children.inc) |
| Ordinary module MP and metadata | [module_data.inc](../fast/module_data.inc), [module_retirement.c](../fast/module_retirement.c) |
| Array and object claims | [array_children.inc](../fast/array_children.inc), [object_children.inc](../fast/object_children.inc) |
| Closed graph publication and release | [graph_publication.inc](../fast/graph_publication.inc), [graph_release.inc](../fast/graph_release.inc) |
| Shared Fast reclamation | [shared_release.inc](../fast/shared_release.inc) |
| Remaining stopped release path | [retained_release.inc](../fast/retained_release.inc) |
| Cycles publication and queue transfer | [candidate_publication.inc](../cycles/candidate_publication.inc), [candidate_transfer.inc](../cycles/candidate_transfer.inc) |
| Mandatory owner rescue to Cycles | [owner_rescue.inc](../cycles/owner_rescue.inc), [packet_rescue.inc](../cycles/packet_rescue.inc) |
| Cycles discovery and entry | [candidate_scan.inc](../cycles/candidate_scan.inc), [rc_passes.inc](../cycles/rc_passes.inc) |
| Duke rollback, rooted queue owners and entry | [pin_release.inc](../duke/pin_release.inc), [rooted_candidates.inc](../duke/rooted_candidates.inc), [rc_passes.inc](../duke/rc_passes.inc) |

Each fragment and its coordinator stay within 500 lines. Include the entire
`gc` source directory when distributing runtime sources. CMake's compiler include
dependencies rebuild `runtime_rc.c.o` when any fragment changes. No generated source
step or new build prerequisite is required. `rc_test_source.py` expands only this
explicit composition for isolated semantic mutation tests; it does not edit the
production files or replace a runtime allocator/collector.

`graph_capture.c` separately composes [graph_classification.inc](graph_classification.inc)
for complete-capture reachability and fresh Duke owner checks. That fragment has
no independent state; it does not participate in `runtime_rc.c` expansion.

The size/layout check does not certify every historical comment or every GC
protocol. Current stop-free scope and outstanding work are described in
[FAST-FINALIZATION.md](../../FAST-FINALIZATION.md).

[Cycle-owner rescue](../../CYCLE-OWNER-RESCUE.md) describes shared seed publication,
root-preserving mandatory handoff and the duplicate-index lifetime boundary.

[Ordinary module retirement](../../MODULE-RETIREMENT.md) describes the MP root
transaction, registry/metadata release and executable-code reader cutoff.

[Packet cycle retirement](../../PACKET-CYCLE-RETIREMENT.md) covers failed child
publication, snapshot replay and rooted ticket staging before discovery.

[Module-instance cycles](../../MODULE-CYCLE-RETIREMENT.md) add ordinary MP edges
and known native finalization after the graph claim and final reader epoch.

[Original-MP closure](../../ORIGINAL-MP-CYCLES.md) adds sparse native attribution
and [native graph claims](../fast/module_graph_claim.inc) to Fast confirmation.
That fragment composes into `graph_confirmation.c`, not `runtime_rc.c`; copied
private confirmation sources must preserve its include directory.

[Multi-owner native closure](../../MULTI-OWNER-MODULE-CYCLES.md) uses
[module_graph.c](module_graph.c) for private discovery indexing, native counts
and live-member propagation. Snapshot copying filters ownership edges and marks
the final graph-owned Modlink for each claimed Module.

[Duke pin retirement](../../DUKE-PIN-RETIREMENT.md) preserves abandoned nonfinal
candidate pins as rooted mandatory seeds. FIFO refusal leaves the journal cursor
unchanged and retries under shared admission.

[Duke allocation retry](../../DUKE-ALLOCATION-RETRY.md) propagates temporary
snapshot allocation pressure through candidate busy state. Hard byte limits
remain distinct; private growth/storage replacements preserve include paths.

[Completed publication retry](../../DUKE-PUBLICATION-RETRY.md) marks temporary
graph metadata refusal for shared retry. All preparation allocations precede the
retirement epoch; failure releases native buffers and cancels the reservation.

[Native claim storage](../../FAST-CLAIM-STORAGE.md) sizes private native vectors
by actual Module entries and propagates allocation BUSY through confirmation.
Private confirmation copies must retain the `module_graph_claim.inc` include.

[Mixed snapshot retry](../../MIXED-SNAPSHOT-RETRY.md) keeps the graph retry
decision in the shared pass result independently of retained queue readiness.
