# Preallocated native root groups

This standalone example runs WITHOUT a managed collector. Two ordinary C objects
stay alive until the complete group is cleared. A reservation creates metadata,
not strong owners, mapped-pointer permission or managed Heap objects. Never pass
these native keys to a hosted collector. For real managed objects see the actual
runtime diagnostic and full [ROOT-BATCH contract](../../runtime/ROOT-BATCH.md).

From the checkout root:

```sh
cc -std=c99 -O2 -pthread -IdisVM/runtime -IflashVM/ports \
  disVM/examples/native-root-batch/main.c disVM/runtime/root_batch.c \
  disVM/runtime/root_registry.c flashVM/ports/linux/channel_sync.c \
  -o build/native-root-batch-example
./build/native-root-batch-example
```

Installed SDK, from package root:

```sh
cc -std=c99 -O2 -pthread -Isource/threading -Isource/ports \
  examples/native-root-batch/main.c source/threading/root_batch.c \
  source/threading/root_registry.c source/ports/linux/channel_sync.c \
  -o native-root-batch-example
./native-root-batch-example
```

The group publishes atomically to locked registry visitors, clears only its own
occurrences and rejects destruction while active. Publish/clear do not allocate;
registry mutex acquisition may wait. Reserve before acquiring access locks, keep
all independent referent owners through clear, and destroy after external
quiescence. Each batch needs one exclusive caller; different batches may race.
The process-lifetime registry mutex and ordinary reusable nodes are not destroyed
by batch teardown. Hosted publication additionally requires its existing GC/
NativeWorld protocol; no concurrent managed execution is enabled by this API.

Validation: `python3 disVM/runtime/test_root_batch.py` (SDK:
`python3 source/threading/test_root_batch.py`). The native source and all private
headers/ports are installed. No external Inferno installation is required.
