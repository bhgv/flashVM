# Compact three-span native access

The commented C99 example adds two unaligned native64 integers into a result that
partly overlaps its first input, checks 42 and surrounding canaries, then copies
the full buffer through the existing wide API. Both access kinds use the same
canonical domain. Buffers/domain remain independently owned through teardown;
no managed Heap/root is fabricated or acquired by these guards.

From a Linux64 checkout:

```sh
cc -std=c99 -O2 -pthread -IdisVM/include -IdisVM/host -IflashVM/ports \
  disVM/examples/native-small-range/main.c disVM/host/small_range.c \
  disVM/host/range_access.c disVM/host/cell_access.c \
  disVM/host/object_guard.c disVM/host/guard_registry.c disVM/host/refcount.c \
  disVM/host/native_context.c disVM/host/native_owner.c disVM/host/native_world.c \
  flashVM/ports/linux/thread.c flashVM/ports/linux/channel_sync.c \
  -o build/native-small-range
./build/native-small-range
```

Installed runtime/editor SDK: use `-Iinclude -Isource/host -Isource/ports`, replace
`disVM/host/` with `source/host/`, `flashVM/ports/` with `source/ports/`, and use
`examples/native-small-range/main.c`. Keep cell_access_private.h with the host
sources; the installer supplies this closure. Expected output starts
`PASS compact native arithmetic:42`.

DVM_TripleAccess has capacity for six canonical boundary stripes, deduplicated and
claimed in ascending order. All three lengths must be 1..sizeof(void*); empty or
larger spans refuse unchanged. Use DVM_RangeAccess for empty/arbitrary-size spans.
The new type does not change existing native access layouts or any Dis64 IDs.
No per-call allocation occurs. BUSY rolls back all guards/domain borrow before
retry; do not wait while holding an access. Metadata locks can wait.

The example has no competing threads; the native suite mixes compact, wide and
cell access on eight real joined OS threads. Run `python3 disVM/tests/small_range.py`
or installed `python3 source/tests/small_range.py`. These are native payload tests,
not general parallel managed execution. Hosted arithmetic still requires the
existing managed owner/root/NativeWorld protocols. See
[SMALL-RANGES.md](../../runtime/SMALL-RANGES.md), installed at SDK root.
