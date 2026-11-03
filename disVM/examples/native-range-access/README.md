# Native byte-range guards

This example owns two native arrays, copies 3000 bytes between unaligned spans,
then checks both the bytes and the untouched boundary canaries. Both arrays stay
alive until all access/domain/context state has retired. No managed Heap or GC
root is fabricated; these guards do not acquire pointed-to ownership.

## Native Linux build

Run from the checkout root:

```sh
cc -std=c99 -O2 -pthread -IdisVM/include -IdisVM/host -IflashVM/ports \
  disVM/examples/native-range-access/main.c disVM/host/range_access.c \
  disVM/host/cell_access.c disVM/host/object_guard.c disVM/host/refcount.c \
  disVM/host/native_context.c disVM/host/native_owner.c disVM/host/native_world.c \
  flashVM/ports/linux/thread.c flashVM/ports/linux/channel_sync.c \
  -o build/native-range-example
./build/native-range-example
```

For an installed runtime/editor SDK, run at package root with
`-Iinclude -Isource/host -Isource/ports`; replace `disVM/host/` with `source/host/`,
`flashVM/ports/` with `source/ports/`, and the example source with
`examples/native-range-access/main.c`. Keep `cell_access_private.h` beside its
source files; the installer includes the full header/source closure.

## Ownership and contention

Every competing pointer-cell, small-byte or wide-range accessor must use the
same domain. Overlap is computed from complete byte extents. The fixed 256 native
stripes are collision exclusion, not an object/thread quota. Large spans can
claim all stripes and temporarily serialize otherwise unrelated accesses.

Begin sorts/deduplicates the union and tries it without per-call allocation.
BUSY rolls back the entire acquired prefix and domain borrow. Retry only after
that rollback; no wait/VM/error callback or context transition may span an access.
A native availability hint is not reservation. Empty spans ignore their address,
but a successfully acquired empty access still owns a domain borrow until end.

End requires exact current context and held depth. A domain count does not
protect racing first lookup/destruction: the creator must exclude new users and
join or otherwise quiesce existing users before destroy. The native buffers and
their independent owners must stay alive through every operation and retry.

## Hosted profile

`disvm --gc-worker --guarded-copies --jit module.dis` opts into the five checked
x64 copy tails. Use `--no-jit` for the C path; other native backends currently
refuse the guarded JIT profile. Normal scalar fast paths stay the default until
local/shared selection and the remaining memory protocols are ready. SELF uses
MOVP's checked ownership path independently of the byte-copy flag.

The flag does NOT enable parallel managed execution. Arithmetic writers,
reference-rich aggregate retirement, source/container lifetimes, GC publication,
scheduler/channels and code lifetime still require their protocols. Raw copying
must not replace managed retains/marks/drops. See the full
[SHARED-BYTES.md contract](https://github.com/bhgv/flashVM/blob/master/disVM/runtime/SHARED-BYTES.md),
also installed at package root.

Native test: `python3 disVM/tests/range_access.py`, or installed
`python3 source/tests/range_access.py`. It checks actual native cell/range mixing,
wrap/full/empty sets, allocator-free intervals, OS threads and strict controls.
