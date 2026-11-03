# Native pointer-cell range access

A pointer-sized value may occupy two aligned cells when its address is unaligned.
Locking only its starting address fails for partially overlapping copies. This
example initializes two private unaligned slots, claims both complete ranges,
copies a caller-owned native pointer and releases the access before using the
result or retiring native context/domain state.

The stack independently owns the pointed-to integer through every operation.
Cell guards do not retain referents, register GC roots, validate mapped pointer
provenance or authorize arbitrary Dis heap access. Do not use raw native copies
as a substitute for the hosted managed MOVP retain/mark/retire protocol.

## Build on native Linux

From the checkout root:

```sh
cc -std=c99 -O2 -pthread -IdisVM/include -IflashVM/ports -IdisVM/host \
  disVM/examples/native-cell-access/main.c disVM/host/cell_access.c \
  disVM/host/object_guard.c disVM/host/refcount.c disVM/host/native_context.c \
  disVM/host/native_owner.c disVM/host/native_world.c \
  flashVM/ports/linux/thread.c flashVM/ports/linux/channel_sync.c \
  -o build/native-cell-access-example
./build/native-cell-access-example
```

Expected: `PASS native cell access example: unaligned source/destination, payload=7`.
For an installed runtime/editor SDK, run at package root with
`-Iinclude -Isource/ports -Isource/host`, replace `disVM/host/` with `source/host/`,
`flashVM/ports/` with `source/ports/` and the example path with
`examples/native-cell-access/main.c`. Choose any writable output path.

## Threaded use

All competing accessors must use the SAME domain. Fixed256 guard stripes are
collision exclusion, not a limit on objects or threads. Collisions can serialize
unrelated slots. Begin computes up to four boundary stripes, sorts/deduplicates
and claims them without per-call allocation. BUSY releases every partial claim
and domain borrow; it does not reserve a future turn. Refused nonzero access
storage is unchanged. Start with zeroed, noncopyable owner-local access storage.

A bare native caller may retry BUSY only after the failed begin has rolled back
all guards. Do not wait/reenter the VM while holding an access. ACTIVE VM callers
need scheduler/root publication policy for a future retry; the current hosted
MOVP fails closed on unexpected contention rather than sleeping with admission.
Domain destruction requires externally quiescent users; its borrow count cannot
protect a race with the first lookup. Keep both mapped ranges and their own
storage owners alive through every claim/end/retry.

Run `python3 disVM/tests/cell_access.py` from the checkout or
`python3 source/tests/cell_access.py` from the installed package. These compile
actual guards/TLS/ports and check unaligned overlap, order, rollback, foreign end,
allocation failures and native multiwriter invariants. See the full
[SHARED-POINTER.md contract](https://github.com/bhgv/flashVM/blob/master/disVM/runtime/SHARED-POINTER.md),
also installed at package root.
