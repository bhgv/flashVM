# Cooperative table leases across channel waits

[Русский](README.ru.md) · [Payload ownership](../../PAYLOAD_GUARDS.md)

`main.b` is a concurrent Limbo acceptance tutorial. `$Bigtable.acquire(tab)`
retains a recursive, process-owned table lease until matching `release(tab)`.
The owning process can read and write the table, call other code and suspend on
channels. A competing mutable-table call parks its Dis process before native
MCALL publishes the frame; the sole executor keeps running other processes.
The pending typed frame remains a GC root and retries the same call after wakeup.
`guardstate` reads metadata without waiting for the payload lease.

The tutorial checks nested ownership, a parked competing writer, diagnostic
progress while the owner waits, rejection of foreign release, caught-exception
cleanup, and retirement of an otherwise unreferenced leased table. With quantum
1, collector polls exercise the pending frame and retained owner's roots.

Build `disVM/runtime`; CTest compiles `tableleases-test.dis` with the matching
native-cell bootstrap compiler. From the repository root:

```sh
build/runtime/disvm --jit --quantum 1 --stats build/runtime/tableleases-test.dis
build/runtime/disvm --no-jit --quantum 1 --stats build/runtime/tableleases-test.dis
```

Expected output starts with `PASS cooperative table leases`; final counters are
`tables_live=0 keys_live=0`. CTest also runs both modes at normal scheduling.

An exception aborts **all** leases of the throwing process before handler lookup,
even when that handler catches it. Reacquire explicitly after recovery. Normal
process exit also releases leases and wakes waiters. Acquire distinct retained
owners in increasing native guard order; an inverse acquisition or inverse
conflicting wait raises `table lease order violation` instead of deadlocking.
Applications should normally retain one destination at a time. These addresses
are internal ordering keys, never portable capabilities or network identities.

This API protects local native Bigtable payloads. [Automatic source assignments](../../../fir/v1/doc/TABLE_LEASES.md) now use it.
It does not yet protect arrays/mutable string slots,
or implement distributed leases, timeouts or reconnection. A program can still
deadlock at the application level by waiting on a channel whose peer needs its
leased table. Metadata inspection cannot repair that protocol.
