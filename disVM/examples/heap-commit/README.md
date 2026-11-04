# Prepared owner publication: a complete hosted C worked example

The fully commented C `scenario` function in
[test_heap_commit.py](../../runtime/test_heap_commit.py) is the worked example.
It runs against actual String/List/Type/collector objects, not simulated native
keys. The Python driver compiles that C with matching runtime objects/flags.
The SDK installs it under source/threading/test_heap_commit.py. It needs a
matching build/object closure for private diagnostic relinking; it is not a
standalone native program or a new Limbo opcode.

Run from the source checkout:

```sh
python3 disVM/runtime/test_heap_commit.py RUNTIME LIMBO disVM/upstream/module
```

Read `scenario` in this order:

1. Construct/pin real Strings before later managed allocations. Build a genuine
   Tptr List container and root it. Its data field is the mapped owning destination.
2. Capture an observed old owner and an incoming owner with heap_hold. Those
   holds retain and root values independently before temporary construction pins
   and original owners are consumed. H uses active-empty reservations.
3. `commit` calls heap_commit under allocation/free/managed-RC/destructor
   prohibitions. The slot is excluded for compare, edge mark, store and root retarget.
4. Success moves incoming's strong owner into the container. Its existing root
   reservation moves to retired and now covers the displaced slot owner. Incoming
   fields are zero; observed remains owned. No managed count is added/subtracted.
5. The stale case uses actual compliant MOVP to change the destination between
   capture and commit. A stale commit preserves both holds/output/cell. This
   example's independent String candidate can retry with a refreshed observation.
   A constructor whose candidate contains the OLD tail must rebuild/refit it first.
6. Run actual Cycles while the new value is reachable only through the rooted
   mapped container and displaced values remain explicitly held. After unlock,
   close observed/retired, release container/remaining pins, drain actual Fast
   and require all descriptor baselines to return.

The fixture also checks same-identity replacement, H→value/value→H/H→H, BUSY,
mark refusal, alias/geometry refusal and unchanged root-retarget refusal. For
retarget refusal the temporary store is restored before unlock. Seven controlled
actual-source omissions must fail precise semantic predicates before unsafe
collection/cleanup. These are test-only deliberate defects, never production
hooks. Signals/compiler/resource/sanitizer failures are not accepted controls.

A successful commit is final. If later consumer/destructor code raises an error,
do not treat the operation as stale and retry publication: incoming is already
consumed and retired already owns the displaced value. Cleanup must consume
those actual live leases once outside guards. See the
[protected callback example](../heap-hold/README.md) for consuming error handling.

Do not copy live leases, fabricate Heap headers, use an unrooted native byte cell
as a substitute for the destination map, or transfer pointers across VM heaps.
NativeWorld still applies. This fixture uses deterministic interleaving, not
simultaneous managed writers. [HEAP-COMMIT.md](../../runtime/HEAP-COMMIT.md)
documents the internal C API, caller lifetimes and limits; it is installed at SDK root.
