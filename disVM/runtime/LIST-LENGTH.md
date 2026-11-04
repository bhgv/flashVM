# Continuous list traversal — runtime0.103

Explicit `--gc-worker --guarded-copies` routes existing LENL through
`shared_list_length.c` and its actual x64 tail. The handler captures the source
only after source/output exclusion. Each discovered tail is added while every
previous link remains held. Thus compliant writers cannot detach the captured
reachable prefix before scalar publication. No temporary owner/root allocation,
retain/destructor, safepoint or VM callback occurs. Current caller-proven container/
referent/root lifetime and NativeWorld/collector exclusion remain mandatory.

H terminates a proper/empty list; NULL is an exNilref error. Native WORD maximum,
field-wrap bounds and Brent checkpoints preserve retained LENL behavior. Cycles
give exHeap, without extra unguarded checkpoint reads. Admission/extension BUSY
or refusal also gives exHeap. Every error propagates only after complete end;
output remains unchanged before publication. No allocator/type validation,
reference-count uniqueness inference or concurrent collector permission follows.

## General native span extension

`dvm_range_access_extend(access, span, size)` adds an independently live span to
the same exact-owner/domain borrow. Empty spans ignore their address; wide spans
can select all256 stripes. Small spans use boundary keys and binary search of the
sorted original subset; larger spans use the existing canonical selector. New
guards are try-only, including keys below prior keys. Native mutex fallback/wake
contracts remain; this is not a wall-clock guarantee or an object-owner park.

Failure releases only additions, preserving the original live access and metadata.
Caller MUST end it before retry, wait, world transition or VM/error propagation.
Success keeps all claims held and merges the metadata into canonical sorted order;
end releases reverse canonical order. This differs from compact six-key extension,
which appends in acquisition order: the handle types must not be cast/copied.
Existing begin/end semantics and struct layouts remain unchanged. Domain borrow
and managed-root/ownership permissions are not extended by this metadata API.
There is no native allocation;256 is stripe capacity, not a thread/object quota.

A long traversal can cover much of the domain and cause unrelated access to
return BUSY. The operation has no internal safepoint, as with retained LENL.
Returning a scalar avoids an escaped element-address lease; other address-returning
instructions and graph/root/GC epochs remain separate work before parallel admission.

## Evidence and reproduction

Native/forced-lock tests use actual domains, guards, TLS and ports:opposed owners,
free-then-busy partial rollback, foreign-owner refusal, sorted merge, empty/live-
empty and full-domain extension, dedup/geometry/single-borrow retirement. Eight
joined OS writers complete16000 coherent paired updates. Direct allocator hooks
observe no malloc/calloc/free in intervals. Bounded atomic test handshake is not
an OS barrier/park under guards. Four implementation omissions per backend must
fail semantically before unsafe payload access; compile/crash/timeout never counts.

The real LENL fixture constructs proper cons chains with caller roots maintained
across allocation. It checks3840 unaligned/overlap cases,128/256/512-node/hash/Brent
boundaries and real cycles including self/two-node cycles. Extra cycle owners are
retained then explicitly unlinked/consumed; malformed NULL topology is restored
before any GC. H/NULL, geometry, output canaries, no temporary owners/roots,
continuous exact field exclusion and admission refusal are observed. A rooted
64-node chain survives32 actual Cycles and final Fast retirement. No fake Heap
headers or native-key roots enter hosted collection.

The real guest reaches one actual x64 LENL tail between two Sys prints. Coverage,
code unmap, inherited C return-PC, error/combined and bad-CALL/old-emitter controls
run under both poll modes. Native C/forced-lock and hosted positives pass TSan
separately; generated machine code is uninstrumented. Exact completed gates are
in [v0.1801](../../docs/releases/v0.1801.md); no physical ARM64/RV64 acceptance is
inferred. Native WORD64 is not shared Frame64 Int32/Ref64 semantics.

```sh
python3 disVM/tests/range_extend.py
CFLAGS='-O1 -g -fsanitize=thread -fno-omit-frame-pointer' python3 disVM/tests/range_extend.py --positive-only
python3 disVM/runtime/test_shared_list_length.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/test_list_length_tail.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_list_length.py RUNTIME LIMBO INCLUDE_DIR
```

Runtime scripts need matching native source/object closure. Native range_extend.py
also detects installed SDK paths. Public range headers compile C89/C++98.
[Limbo example](../examples/guarded-list-length/README.md),
[C example](../examples/native-range-extension/README.md),
[cost ledger](PERFORMANCE-COSTS.md). Data/branch tail selection67→68 is not a
readiness percentage. NativeWorld still serializes general managed execution.
