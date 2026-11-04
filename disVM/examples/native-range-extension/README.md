# Keeping a native chain protected through a scalar snapshot

The fully commented example separates the walker (`walker.c/h`) from its caller
(`main.c`). Main owns two stack nodes, a domain and a bare native TLS context.
The walker guards the source/output first, then adds each discovered tail field
without releasing any earlier guard. It publishes only the final scalar count.
Brent checkpoints detect a cycle without extra unguarded link reads.

The demonstration checks a two-node chain, an empty chain and a temporary cycle.
Cycle refusal preserves output, and every path ends its live access. This native
example uses NULL termination and REFUSED as a demonstration status; actual Dis
LENL has its own H/NULL distinction and VM errors. No fake Heap or GC root is used.

Source checkout build:

```sh
cc -std=c99 -pthread -I disVM/include -I flashVM/ports \
  disVM/examples/native-range-extension/main.c \
  disVM/examples/native-range-extension/walker.c \
  disVM/host/range_access.c disVM/host/cell_access.c disVM/host/object_guard.c \
  disVM/host/refcount.c disVM/host/native_context.c disVM/host/native_owner.c \
  disVM/host/native_world.c flashVM/ports/linux/thread.c \
  flashVM/ports/linux/channel_sync.c -o build/native-range-extension
./build/native-range-extension
```

Installed runtime/editor SDK root:

```sh
cc -std=c99 -pthread -I include -I source/ports \
  examples/native-range-extension/main.c examples/native-range-extension/walker.c \
  source/host/range_access.c source/host/cell_access.c source/host/object_guard.c \
  source/host/refcount.c source/host/native_context.c source/host/native_owner.c \
  source/host/native_world.c source/ports/linux/thread.c \
  source/ports/linux/channel_sync.c -o native-range-extension
./native-range-extension
```

Expected:`PASS native chain snapshot, empty and cycle refusal`.
This Linux native-data example creates its own bare context; a real module uses
its already-bound host context and independent managed admission. Registration is
shown by the existing [native-module example](../native-module/README.md).

`dvm_range_access_extend` preserves the original access on BUSY/refusal. End it
before any retry, park, VM/error callback or world transition. New guards are
try-only even below old keys. Successful metadata stays sorted; end releases
reverse canonical order. Native mutex fallback/wake contracts remain; no bounded
wall-clock wait is promised. Empty spans ignore addresses, and wide spans can
cover all 256 canonical stripes. This is not a thread/object quota.

All reachable storage remains caller-owned throughout, and competing writers must
use this same domain. A guard is not provenance, a strong reference or a root;
no concurrent collector admission follows. For actual native/forced-lock OS-thread
coverage run `python3 source/tests/range_extend.py` in the SDK (or
`python3 disVM/tests/range_extend.py` in the checkout).
