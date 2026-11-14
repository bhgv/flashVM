# Inspect live memory accounting from Limbo

Copyright (c) 2026 bhgv

[Русский](README.ru.md) · [Contract](../../runtime/MEMORY-STATISTICS.md)

The example starts a real resident, prints snapshots before/during/after a byte
allocation, then joins the guest. Other owners are not paused.

```sh
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/module \
  -o build/disvm-opt/memoryview.dis disVM/debug/memoryview.b
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/module \
  -o build/disvm-opt/memory-statistics.dis disVM/examples/memory-statistics/main.b
build/disvm-opt/disvm --stats --no-jit build/disvm-opt/memory-statistics.dis
build/disvm-opt/disvm --stats --jit build/disvm-opt/memory-statistics.dis
```

Keep memoryview.dis beside the caller. Success prints `PASS memory statistics example`.
Check `arena = header + live + free`. After `bytes = nil`, live may still increase:
reclamation is deferred, and printing/Stats also allocate memory. These spans are not
requested payload or RSS. Capacity includes alignment and Heap headers; free blocks are
not coalesced. Per-thread/per-type attribution remains separate work.
