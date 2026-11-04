# Discovering a field under continuous source protection

The commented C program shows the new native `dvm_small_range_access_extend` API.
Main owns a record until all operations finish. `read_length` first guards its
reference cell and output, captures the pointer, then try-claims the discovered
field while keeping every original guard. It publishes only after complete
exclusion. A nil reference gives zero. Failed extension leaves the original access
live: the example explicitly ends it before returning BUSY/REFUSED.

Build from the source checkout:

```sh
cc -std=c99 -pthread -I disVM/include -I flashVM/ports \
  disVM/examples/native-small-extension/main.c \
  disVM/host/small_range.c disVM/host/cell_access.c disVM/host/object_guard.c \
  disVM/host/refcount.c disVM/host/native_context.c disVM/host/native_owner.c \
  disVM/host/native_world.c flashVM/ports/linux/thread.c \
  flashVM/ports/linux/channel_sync.c -o build/native-small-extension
./build/native-small-extension
```

Installed runtime/editor SDK root:

```sh
cc -std=c99 -pthread -I include -I source/ports \
  examples/native-small-extension/main.c \
  source/host/small_range.c source/host/cell_access.c source/host/object_guard.c \
  source/host/refcount.c source/host/native_context.c source/host/native_owner.c \
  source/host/native_world.c source/ports/linux/thread.c \
  source/ports/linux/channel_sync.c -o native-small-extension
./native-small-extension
```

Expected:`PASS native continuous source/field snapshot`.
The test scope is native Linux storage. The program creates a bare context for
this standalone process; a real VM module uses the already-bound host context.
This does not grant Dis heap access, create roots or permit cross-thread jumps.
See the existing [native-module example](../native-module/README.md) for module
registration, which this guard API does not change.

New keys may precede old keys numerically because acquisition is bounded try-only,
with no object-owner wait. Metadata mutexes retain the existing platform contract.
A failed partial addition releases only the new guards, preserving the original
prefix and its one domain borrow. There are at most six unique stripes, including
unaligned boundaries. Callers retry only after ending the entire access; they
must not park, call the VM or propagate an error while retaining it. Every writer
must cooperate through the same domain, and source/referent lifetime is supplied
independently. No guard creates pointer provenance or collector admission.
