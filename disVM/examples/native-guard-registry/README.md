# Canonical native object guards

Two independent captures of the same live identity must not create two locks:
that would allow simultaneous writes to the same object. This example acquires
two native leases for one stack-owned integer, verifies canonical guard identity,
performs guarded work, then retires leases, registry and TLS in that order.
The payload stays independently alive through BOTH leases. A lease owns native
sidecar metadata only; it neither retains an arbitrary key nor creates a GC root.

## Build from the checkout on native Linux

Run from the repository root; no Inferno installation or external VM is required:

```sh
cc -std=c99 -O2 -pthread -IdisVM/include -IflashVM/ports -IdisVM/host \
  disVM/examples/native-guard-registry/main.c \
  disVM/host/guard_registry.c disVM/host/object_guard.c disVM/host/refcount.c \
  disVM/host/native_context.c disVM/host/native_owner.c disVM/host/native_world.c \
  flashVM/ports/linux/thread.c flashVM/ports/linux/channel_sync.c \
  -o build/native-guard-registry-example
./build/native-guard-registry-example
```

Expected: `PASS native canonical guard example: payload=7`.

For an installed disvm-editor/disvm-runtime SDK, run from the package root and
use `-Iinclude -Isource/ports -Isource/host`; replace source prefixes `disVM/host/`
with `source/host/`, `flashVM/ports/` with `source/ports/`, and the example path
with `examples/native-guard-registry/main.c`. All native source/header dependencies
are installed together. The output path can be any writable local path.

## Contention and lifetime

The example has no competitor, so try-enter must acquire immediately. In threaded
code, BUSY is not permission to access payload. A caller that can safely park may
wait for availability and retry; availability does not reserve the guard. ACTIVE
VM owners must publish roots and leave their interval before parking. No guard
may span that transition. See `object_guard.h` for exact role restrictions.

Fetch all borrowed guard pointers before taking any guards. The lease and its
independently owned key must remain live through try/park/retry/leave. Release a
lease only after leaving all guards. Final release removes the canonical node;
its pointer must never be reused, even if a later object receives the same address.
Registry destruction requires external quiescence, not merely an empty snapshot.

## Hosted managed integration and evidence

Hosted managed tickets now acquire one lease per capture, keyed by referent in
one runtime-local registry. Their real strong reference plus external root keep
the key live. Fast releases the lease BEFORE removing that root and destroying
the managed owner. Separate captures share a guard; ticket aliases do not create
extra sidecars. This does not enable unguarded Heap reads from bare native code:
`dvm_managed_owner_value` still requires serialized ACTIVE admission, and borrowing
its guard does not grant new payload access permission.

`python3 disVM/tests/guard_registry.py` checks eight OS writers, sixteen thousand
guarded native writes, hash collisions, overlapping first captures, native and
forced-lock backends, every constructor allocation refusal and strict controls.
Installed equivalent: `python3 source/tests/guard_registry.py`.
The separate actual-runtime diagnostic checks managed captures, root-error
rollback, partial Fast, rooted Cycles and final retirement. Its private relink
requires matching GNU Makefiles runtime objects. Read the full contract in
[GUARD-REGISTRY.md](https://github.com/bhgv/flashVM/blob/master/disVM/runtime/GUARD-REGISTRY.md). The installed contract is also at the package root.
