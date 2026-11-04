# Continuously guarded array length — runtime0.102

Explicit `--gc-worker --guarded-copies` now routes existing LENA through
`shared_array_length.c` and the real x64 tail dispatcher. Default handlers and
emission remain intact after a full-body drift check. No shared Dis64 wire/IDs,
ordinary WORD32 behavior, module ABI or public execution admission changes.

## Source lifetime without a temporary owner

The handler first guards the live source reference cell and WORD output in the
canonical compact domain. `array_length_value.c` captures the pointer only after
that claim. Nil/H references publish0 without reading a referent. A nonnil pointer
is checked for length-field wrap, then its field is try-claimed by extending the
same live access. Every initial guard remains held through capture, discovered
field read and result publication. Partial source/output aliases are snapshot-
safe; a length field already covered by the initial union is deduplicated.

This path allocates no temporary roots, retains or snapshots. The existing source
owner remains protected against compliant slot writers until its last use. Caller
still supplies mapped cells, a valid referent and root/lifetime exclusion. A guard
is not pointer provenance or a new root. The instruction has no VM callback,
allocation, destructor or object-owner wait while held; native metadata keeps its
existing mutex contract. NativeWorld and current stopped-world collector rules
remain. This is not permission for concurrent graph collection or raw-pointer use
across a safepoint. The handler preserves the original native WORD field bits;
it does not establish the shape or validity of an arbitrary allocation, inspect
reference counts or infer COW uniqueness.

Source/output geometry failures produce exBounds. A wrapped nonnil field address
also produces exBounds; native admission/extension BUSY or refusal gives exHeap.
All errors propagate only after ending the complete access, including the original
prefix retained by a failed extension. Output is unchanged before publication.

## New native extension contract

`dvm_small_range_access_extend(access, span, size)` adds one1..pointer-width mapped
span to an existing exact-owner `DVM_TripleAccess`. There are at most six unique
canonical stripes per access; this is not a thread/object quota. The initial prefix remains sorted; new keys are deduplicated
against it and tried in ascending order, then appended in acquisition order.
Keys may be lower than prior keys: no object-owner wait occurs, so opposing
extensions return BUSY rather than blocking each other. Metadata fallback/wake
mutexes have their existing bounded-scope contract, not a wall-clock guarantee.

Success preserves the single domain borrow and all old claims. On BUSY/refusal,
only newly added claims are released in reverse order; the original access stays
live and unchanged. Caller MUST end it before retry, error propagation, a world
transition, VM callback or wait. Capacity/geometry/foreign/depth refusal also
preserves that prefix. Begin still rolls back everything on failure; the shared
result enum comment now distinguishes those two uses. Struct layout and existing
begin/end semantics are unchanged. No native allocation or managed retain/root operation
is introduced. See [C example](../examples/native-small-extension/README.md).

## Evidence and limits

The actual native-domain fixture runs native and forced-lock backends. Two OS
owners first hold opposite stripes; one tries a free new stripe followed by a
busy stripe, testing partial rollback. Both must retain only their original
prefix. Foreign-owner extension refuses during a separately synchronized stable
phase. Eight joined writers then make16000 coherent paired updates, retrying only
after full end. The fixture also checks six-stripe capacity, dedup, invalid/wrapped
geometry, single-borrow retirement and no malloc/calloc/free inside the intervals.
Handshake polling is bounded native test code; no OS barrier/park occurs with
held guards. Three strict implementation omissions per backend require semantic10
before unsafe data access; compiler/crash/timeout failure never counts.

The actual LENA fixture uses real heaparray objects of four element types,15360
unaligned/overlap cases, actual source/field capture hooks, canaries and unchanged
owner/root counts. Nil/H values, malformed slots and extension refusal preserve
semantics and cleanup. Deliberate source/field guard/publication omissions require
specific semantic failures before unsafe continuation. Guards refuse world release.
A caller-rooted mapped array and real child String survive32 actual Cycles, then
retire through Fast after guards close; no forged Heap or native-key GC roots.

The real guest reaches one actual LENA x64 tail. Between-print coverage, individual
code unmap, inherited C return-PC, error/combined and bad-CALL/old-emitter controls
run under both poll modes. TSan C helpers and native backends are separate from
uninstrumented generated code. Exact completed gates and platform scope are in
[v0.1800](../../docs/releases/v0.1800.md). No general parallel managed admission.

```sh
python3 disVM/tests/small_extend.py
CFLAGS='-O1 -g -fsanitize=thread -fno-omit-frame-pointer' python3 disVM/tests/small_extend.py --positive-only
python3 disVM/runtime/test_shared_array_length.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/test_array_length_tail.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_array_length.py RUNTIME LIMBO INCLUDE_DIR
```

Runtime scripts need matching source/object closure. Native small_extend.py is
also installed at `source/tests/` and detects SDK paths. Public headers compile
as C89/C++98. [Limbo example](../examples/guarded-array-length/README.md).
[Measured cost](PERFORMANCE-COSTS.md). Data/branch tail selection66→67 is a native
dispatch count, not a readiness percentage or shared Int32/Ref64 proof.
