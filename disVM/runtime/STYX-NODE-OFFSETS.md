# Positioned VFS callback files

[Русский](STYX-NODE-OFFSETS.ru.md) · [Limbo publication](STYX-NODES.md) · [ActionScript publication](../../flashVM/doc/STYX_RPC_NEWNODE.md) · [C SDK](../../lib/styxrpc/reference/module_slot_new_node_at.md)

Use positioned publication when the callback represents bytes at known
positions in a fixed-length object. The length is in bytes, from zero
through 2⁵³−1, and is reported by ordinary VFS stat. The publication
allocates no storage proportional to length. The callback owns the data
and can serve a sparse file. A cursor starts at byte zero on each open.
The caller can seek from the start, its current position or the end.
The next read or write supplies the absolute position to the handler.

| Publisher | Declaration | Handler |
| --- | --- | --- |
| Limbo | [`nodes->publishat(path, permissions, length)`](styx-nodes-publishat.md) | [`transfer(path, operation, buffer, offset)`](styx-nodes-transfer-at.md) |
| ActionScript | [`vfs.newnode(path, permissions, callback, length)`](../../flashVM/doc/STYX_RPC_NEWNODE.md) | `callback(operation, buffer, len, offset)` |
| C SDK | [`styxrpc_module_slot_new_node_at`](../../lib/styxrpc/reference/module_slot_new_node_at.md) | `transfer(context, access, offset, bytes, size, used)` |

Each read gets a buffer limited to the remaining bytes. Reading at or
beyond EOF returns zero without invoking the callback. A write fragment
crossing EOF fails before dispatch. A request split into transport
fragments can have earlier confirmed writes. A successful short count
advances the cursor only by the number of confirmed bytes. Execution
still requires `x`, receives its original payload independently of
file length, and reports offset zero. The offset is an AS Number or a
Limbo `big`; local DisVM seek also respects the native signed `long`
limit. A remote seek may update a client cursor immediately. The server
sees the new position on the next data request.

The ordinary Styx messages carry all data and positions in the existing
libstyxrpc session. The callback runs in the owner’s VM. Use the
[stream publication](STYX-NODES.md) for content driven by arrival order.

## Paired check

Add `--positioned` to the [paired build command](STYX-CHANNEL.md#acceptance).
The [Limbo entry](../../lib/styxrpc/tests/flashvm/positioned_pair.b) publishes
a file and starts the [AS movie](../../lib/styxrpc/tests/flashvm/positioned_pair.as).
The movie publishes a second file using an
[AS callback](../../lib/styxrpc/tests/flashvm/positioned_transfer.as) and writes
near the end of the first. The [Limbo handler](../../lib/styxrpc/tests/flashvm/positioned_handler.b)
checks offsets above 4 GiB, returns the stored binary bytes and handles
execution. The Limbo entry then performs the same operations against the
FlashVM file using `Sys`. Expected output is
`PASS native positioned RPC paired VMs`. Add `--tcp` or `--jit` for those modes.
