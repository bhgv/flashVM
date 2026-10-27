# Guard release before exception recovery

[Русский](README.ru.md) · [Ownership contract](../../PAYLOAD_GUARDS.md)

`main.b` is a native-cell Limbo program using the signature-checked `$Bigtable`
library. It seeds a scalar table, reads guard metadata, catches an invalid range
and a failed transactional splice, then confirms the object is usable and no
owner/depth/waiter remains. It also self-splices, copies the prefix and keys,
retires the original handle and checks independent snapshot lifetime.

Build through `disVM/runtime`, which generates `tableguards-test.dis`. Run from
the repository root with matching built tools:

```sh
build/runtime/disvm --jit --quantum 1 --stats build/runtime/tableguards-test.dis
build/runtime/disvm --no-jit --quantum 1 --stats build/runtime/tableguards-test.dis
```

Both runs print `PASS table guards, exception cleanup and independent snapshots`.
Statistics finish with `tables_live=0 keys_live=0`. The example is compiled by
the retained native bootstrap to cover full Limbo exception syntax; our bounded
Coco/R source profile remains separately tested for table/string assignments.
This is local native library use, not a network proxy example. See the C request
layer's pthread tests for real simultaneous scalar payload contention.
