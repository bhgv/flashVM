# Prepared pointer-list candidates — runtime0.113

`pointer_prepare.c` composes `heap_hold` inputs with the shared construction
algorithm in `pointer_stage.c`. It prepares a complete unpublished `Tlist` node
with a `Tptr` head, then returns one exclusive `DVM_HeapHold`. This internal native
API is a prerequisite for a later constructor migration, not a new opcode or a
parallel managed allocator. Ordinary ACTIVE/NativeWorld admission is mandatory.

Runtime0.114 follow-up: [guarded constructor integration](POINTER-TRANSACTION.md)
now uses this API from selected hosted CONSP and its existing x64 tail entry.
The implementation and refusal evidence below describe preparation itself;
NativeWorld/allocator/collector exclusion remains mandatory.

## Ownership and ordering

1. The caller captures independently live source and destination-tail cells using
   `dvm_runtime_heap_hold`. These holders protect identity/lifetime, not mutable
   payload contents. Their storage stays exclusively borrowed and unchanged.
2. Pass a zeroed, disjoint candidate output. Source may be any genuine held Heap
   or H; tail must be a genuine held Tlist or H. Source and tail may be the same
   successful input hold. Geometry checks do not prove mapping or provenance.
3. Reserve candidate native root metadata before managed construction. Acquire
   input and Type owners transactionally. Construct a coherent node with tail H,
   an ordinary construction pin and the head already installed and marked.
4. The existing monitor may inspect this pinned node or raise. It must not mutate
   borrowed inputs, retain the private node, or leave managed admission.
5. Link and mark the held tail. Publish a Heap-header root in the reserved group
   before withdrawing the ordinary construction pin. Only then return the output.
6. A preparation failure unwinds the exact acquired stages. In particular, root
   publication failure after linkage releases the linked-tail owner. Rollback
   does not invoke a destructor monitor recursively. The output remains zero.

No destination cell is written by preparation. Use `dvm_runtime_heap_commit`
under its separate rooted-container/mapping/type contract for a single guarded
publication attempt. A stale destination returns zero: **finish the old candidate,
refresh the tail hold, and build a new candidate**. Merely refreshing the observed
identity and committing a candidate containing the old tail is incorrect.
Finish displaced and temporary holds outside object guards. Do not copy a live
hold, reuse its root reservation, or retry a consuming cleanup.

`pointer_builder.c` preserves the old serialized body for unselected/inline
execution. Since runtime0.114, selected worker CONSP and its existing x64 tail
entry use the coordinator in [pointer_transaction.c](POINTER-TRANSACTION.md).
This adds no new machine-code opcode and retains ordinary NativeWorld exclusion.

## Reproduction and worked example

The fully commented C fixture inside `test_pointer_prepare.py` is the worked
native example: it captures input holds, prepares a candidate, interleaves a
compliant MOVP writer, rejects the stale candidate, rebuilds and commits, then
retires all temporary owners. It creates actual Strings and mapped List storage;
no fake Heap or scheduler root is used.

```
python3 disVM/runtime/test_pointer_prepare.py \
  build/runtime/disvm build/install1804/disvm-runtime/limbo-native disVM/upstream/module
```

The fixture checks two native reservation allocation failures, two descriptor
acquisition failures, managed-node allocation failure, monitor error, and root
publication refusal after tail linkage. Three exact-source omission controls
must stop with semantic exit10 before unsafe collection: linked-tail rollback,
linkage itself, and ordinary construction-pin withdrawal. Positive runs cover
C/JIT surrounding guests and both collector polls,32 actual Cycles and final
Fast. These are native helper tests, not evidence of concurrent managed mutation,
new compiled CONSP support, ARM64/RV64 execution or GUI responsiveness.

The older `test_pointer_builder.py` separately retains legacy alias/monitor/
allocation/descriptor rollback coverage. `test_heap_commit.py` separately covers
its guarded publication/rollback invariants. See [Heap commit](HEAP-COMMIT.md),
[Heap holds](HEAP-HOLD.md), and [performance ledger](PERFORMANCE-COSTS.md).
