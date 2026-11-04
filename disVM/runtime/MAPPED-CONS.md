# Guarded typed CONSMP — runtime0.118

Selected hosted CONSMP and its existing aggregate SysV x64 tail now compose
`mapped_prepare.c` and `mapped_cons.c`. Source is a genuine owning record with a
live immutable module Type; destination is an owning Tlist/H slot. Caller preserves
mapped cell/frame/Type/code/root lifetime under ACTIVE/NativeWorld and no object
guards. Geometry/count checks are not provenance or arbitrary-pointer validation.
No general parallel allocator/collector or new serialized opcode/ABI is admitted.

## Typed preparation

`dvm_runtime_mapped_prepare` accepts source, Type, a successful exclusive tail
hold and zero disjoint candidate output. It validates map/extent geometry before
initialization, reserves root metadata, retains tail/Tlist/element Type owners,
allocates a private List and initializes every mapped field to H. An ordinary pin
protects this safely traversable empty graph. Actual `dvm_runtime_mapped_copy`
then captures the full head and retains every non-H occurrence under complete
source/destination range guards; duplicate references get independent counts.

The old destination fields are all H. Therefore mapped-copy failure either rolls
back before publication, or succeeds without outgoing destructors/callbacks. The
constructor records the entire head as owned only after that helper returns.
It links/marks tail, publishes a candidate root and withdraws its ordinary pin
before calling the monitor. The monitor sees a **complete traced graph**, with
actual tail, one candidate root and no object guards. It may inspect/raise or
perform separate compliant application writes, not retain/mutate private nodes/
holds or leave admission. There is no Type/map payload access from borrowed REG
or Module code after callbacks.

Before handoff, failures undo head/tail/Type retains nonfinally: original owning
source/tail have not encountered a callback and still protect all fields. A
post-handoff monitor error may follow source replacement, so it uses consuming
rooted List cleanup, preserving the primary error. It does not manually decrement
possibly-final child owners. Last-owner retirement may remain queued for Fast.

The snapshot freezes record bytes and reference identities, not mutable payloads
behind those references. Referent mutation needs its own access protocol; no deep
copy, RC1 uniqueness or exclusive payload permission follows from these holds.

## CONSMP coordinator and routing

1. Capture the Type index with the guarded native WORD-copy helper, resolve and
   validate the existing module descriptor under ordinary module lifetime.
2. Build a traced immutable head snapshot with H tail. This snapshot owns its
   element Type and all mapped reference occurrences independently of the source.
3. For each attempt hold the current destination, build a candidate from snapshot
   data/Type and that held tail, then use conditional Heap commit.
4. Stale candidates/observations are consumed before refresh/rebuild. At most
   eight candidate preparations; the snapshot stays fixed across application
   source/destination changes, including aliased source/destination cells.
5. Drain candidate/retired/tail/snapshot/empty-tail leases even if one destructor
   raises. Preserve first body error or first cleanup error; no release is retried.
   A cleanup error after commit does not undo application publication.

This first implementation builds **two nodes on the successful ordinary path**
(snapshot then candidate), with a complete monitor event for each allocation.
This cost is intentional and recorded, not hidden as a performance improvement.
Unselected/serial-inline retains the original C body. The exact-source startup
adapter refuses drift/double application; the existing checked aggregate x64
entry now rejects held object guards before operands and tail-enters C as before.

## Native worked fixtures and evidence

`test_mapped_prepare.c` builds real dtype/Heap/String/List graphs. It checks seven
native metadata allocation failures and ten separate pool/pin/descriptor/root/
barrier/retain/monitor failures, including second duplicate retain and source
replacement before a monitor error. Malformed extent/map reject before capture.
H fields acquire no fake owners. After original source/creator/holds are retired
and Fast drains, exact remaining counts prove the candidate graph is the sole
managed owner;32 actual Cycles preserve it, then final Fast restores baselines.
Three scoped source controls detect missing head rollback, leaked construction
pin and missing candidate root at exact semantic10.

`test_mapped_cons.c` checks eight transaction cases/nine initial refusal stages,
alias/H/one-and-repeated stale source preservation and primary/secondary cleanup
errors. Three coordinator source controls detect missing stale retirement,
incomplete unwind and primary-error overwrite. Actual Limbo string+int records
exercise CONSMP in C/JIT and both collector polls. No fake Heap/Type/scheduler root.

```
python3 disVM/runtime/test_mapped_prepare.py \
  build/runtime/disvm build/install1804/disvm-runtime/limbo-native disVM/upstream/module
python3 disVM/runtime/test_mapped_cons.py \
  build/runtime/disvm build/install1804/disvm-runtime/limbo-native disVM/upstream/module
python3 disVM/runtime/test_mapped_tail.py \
  build/runtime/disvm build/install1804/disvm-runtime/limbo-native disVM/upstream/module
```

The last fixture proves actual mapped x64 target foreign/cold REG refusal, current
code unmap, VM error, combined unmap/error and strict bad-CALL detection. After a
monitor error, one typed snapshot may remain rooted in the actual Fast queue;
it is counted rather than incorrectly asserting immediate descriptor destruction.
Native-C TSan and installed fixture/source closure are separate evidence. No
machine-code TSan, physical ARM64/RV64/Android/full IDE or GC-during-parallel-mutation
claim follows. [Cost ledger](PERFORMANCE-COSTS.md) records both preparation and
coordinator; [shared mapped copy](SHARED-MAPPED.md) keeps its unchanged contract.
