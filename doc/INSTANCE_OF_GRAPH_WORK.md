# Budgeted interface graph membership — v0.381

Loaded fvm_exec_instance_of no longer rejects a graph after 4096 visited
interface nodes. An identity-keyed hash map replaces the linear visited-vector
scan. Complete pointer equality resolves bucket collisions, including low-bit
hash truncation on LLP64; the requested native ports require size_t to hold a
pointer and compilation checks that assumption. Typical visited lookup is
constant time; no worst-case hash complexity guarantee is added.

An execution-work scope starts before loaded coercions and shares the existing
AVM1 counter with nested script/native execution. Outermost direct calls reset
the counter; nested calls retain it. Every ordinary prototype iteration, pending
node pop (including duplicates) and interface edge is charged before processing
or appending. Budget exhaustion or unsigned counter saturation returns FVM_ERROR
and leaves output intact. A zero instruction limit remains unlimited. Worker
stop polling uses the existing native Array policy once per 64 charged units.
Array callers retain that policy through their old type alias; no Array work
counts are intentionally changed.

[GC:] Existing operand/prototype roots retain the graph during boxing and any
collecting callback. After coercion, traversal invokes no script or collection;
visited keys and pending pointers borrow edges from that rooted graph. Partial
hash/stack allocation unwinds normally on OOM; execution depth is restored on
success, budget failure, callback failure and allocation exception.

Ordinary prototype depth/cycle bounds remain. Interface edges still do not walk
ordinary prototype ancestry. Frozen prototype edges and legacy constructor
edges retain their previous interpretation. Unloaded host calls retain their
strict existing class API policy. No public ABI or transport changes. Other
native helper/property scan costs are not newly accounted by this patch.

Tests: instance_of_large_graph covers SWF4–8 true/false cyclic fan-out graphs
with 8192 interface nodes, GC, aliasing, finite/unlimited/shared budgets and
counter saturation. instance_of_large_graph_allocation sweeps stack/hash
allocation and checks unchanged output, root/depth cleanup, GC and retry.
jit_large_interfaces checks 4097-node graph membership, shared-budget failure
and retry through interpreter, explicit and automatic native code in SWF4–8.
Existing membership callback/boxing/allocation, interface and Array regressions
remain required. All nine roadmap stages remain open.

Validation v0.381: all 792 AGG / 792 Cairo / 773 minimal Linux Release checks
have passing evidence; AGG/Cairo passed initially, minimal required one ELF
execution-permission repair and targeted rerun. Eight restricted heap/global
ASan+UBSan checks pass: three new graph/native checks, existing membership
callbacks/allocation/data and large-sort/negative-end Array regressions.
Stack ASan and LeakSanitizer remain disabled. The new native polling and revised
membership constants headers pass a strict C89 syntax probe. No new original
external SWF oracle or foreign-OS acceptance is claimed. All nine stages remain open.
