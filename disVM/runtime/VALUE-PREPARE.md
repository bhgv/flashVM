# Scalar and unmapped-record candidates — runtime0.115

`value_prepare.c` builds a complete unpublished `Tlist` node with a raw head and
nil element descriptor. Its input is a **private immutable snapshot**, not an
arbitrary shared VM address. The caller must capture shared scalar/raw operands
under their own cell/range exclusion before invoking this API. Embedded managed
references are prohibited: mapped records need a separate typed ownership/map
algorithm. The API does not scan bytes or infer pointer provenance.

Borrow a genuine stable Tlist/H `DVM_HeapHold` tail and zero disjoint candidate
output under ACTIVE/NativeWorld, with stable source/Type/code/root lifetime and
no object guards. A zero-sized payload ignores its address. Six tested extents
are0/1/4/8/17/257 bytes; the general allocation bound is checked before input reads.

1. Reserve native candidate root metadata before managed construction.
2. Acquire extra tail and Tlist owners transactionally; allocate the private node.
3. Initialize header, nil element descriptor, tail H and the complete raw payload.
   Publish an ordinary construction pin before calling the monitor.
4. The monitor may inspect/raise but not retain/mutate the node or borrowed
   inputs, or leave admission. It sees a coherent head, tail H and root.
5. Link/mark the held tail. Publish the candidate Heap root in its reservation
   before removing the ordinary construction pin; return one creator owner/root.
6. On refusal, remove a construction pin if present, manually retire the private
   raw node, return exact tail/Type acquisitions and destroy native reservation.
   Do not invoke a destructor/monitor recursively during error rollback.

Combine the resulting candidate with [conditional commit](HEAP-COMMIT.md) under
its rooted destination and cell-lifetime contract. If the destination is stale,
finish the candidate, refresh tail, and prepare a new one. Inputs remain unchanged;
candidate remains zero on refusal. Geometry checks are not mapping/provenance.
Pending last-owner retirement may require joined Fast before final baselines.

This is an internal preparation API, not CONS B/W/L/F/M integration. The direct
scalar x64 emitter still builds nodes separately; changing only a C opcode would
leave that path outside the protocol. A later coordinator and consistent C/JIT
routing are required. NativeWorld, allocator and collector exclusion stay intact.
No new opcode, feature/ABI ID or serialized Dis64 meaning is introduced.

## Worked C fixture and checks

`test_value_prepare.c` is the complete commented native fixture. It uses real
String/List storage to prove unrelated source ownership remains unchanged, raw
payload comparison, real compliant MOVP interleaving, stale candidate discard,
fresh-tail rebuild and mapped destination publication. After all temporary holds
are closed,32 actual Cycles preserve the published graph, and Fast restores Type
baselines. No fabricated Heap/Type/scheduler root is used.

```
python3 disVM/runtime/test_value_prepare.py \
  build/runtime/disvm build/install1804/disvm-runtime/limbo-native disVM/upstream/module
```

Each of six extents runs both C/JIT surrounding guests and both collector polls.
Seven refusals cover two native reservation allocations, node allocation, Tlist
retain, ordinary construction-pin registration, monitor error and root publication after linkage. Four scoped source
controls detect missing linked-tail rollback, corrupted payload, construction-pin
leak and missing root publication, at exact semantic exit10 before unsafe GC.
Compiler errors/crashes/timeouts/sanitizer reports are never control successes.
C/JIT here does not claim compiled scalar-constructor integration. Native-C TSan
and installed-source closure are separate checks; no ARM64/RV64/GUI or concurrent
collector acceptance is implied. Known costs: [VALUEPREP-115](PERFORMANCE-COSTS.md).

Runtime0.116 follow-up: [scalar C/JIT coordinator](SCALAR-CONS.md) now captures
shared scalar operands before calling this API. Arbitrary raw/mapped record
opcodes remain separate; the private-snapshot contract above is unchanged.

Runtime0.117 follow-up: [raw CONSM](RAW-CONS.md) now captures shared unmapped
source/count into native immutable storage before calling this preparation API.
