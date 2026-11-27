# Shared value cells on a named channel

[Русский](named_cell.ru.md) · [Header](../include/styxrpc/named_cell.h) · [Channels](named_channel.md) · [FlashVM](../../../flashVM/doc/STYX_RPC_NAMED_SEND.md)

A cell belongs to one attached registry. The issuer uses host entropy and a
serial to identify it. Each open cell fid holds a lease. A named channel also
holds the lease while the descriptor is pending and after receiving its
frame, up to the next request or clunk. The receiver opens its own lease before
starting another channel operation. Final release retires the token.

A cell accepts portable number, counted text, Boolean, null and undefined
frames, plus the FlashVM graph snapshot used by ordinary named messages.
A write replaces the complete frame atomically. Successive reads and writes
are separate operations. Serialize operations on each fid. The value may
change type on any assignment. Graph reads return a snapshot.

| C client call | Contract |
| --- | --- |
| [`styxrpc_cell_create`](cell_create.md) | Create a cell and receive its first lease and token. |
| [`styxrpc_cell_attach`](cell_attach.md) | Open a lease from a received token. |
| [`styxrpc_cell_get`](cell_get.md) | Read a complete value frame. |
| [`styxrpc_cell_set`](cell_set.md) | Replace the complete value frame. |

| C call with cancellation | Contract |
| --- | --- |
| [Create with stop](cell_create_until.md) | Transfer the token and first lease only after a complete successful reply. |
| [Attach with stop](cell_attach_until.md) | Attach a fresh fid to a borrowed live token. |
| [Read with stop](cell_read_until.md) | Fetch the value on a separate temporary fid, leaving the original lease available. |
| [Write with stop](cell_write_until.md) | Replace the frame once and preserve uncertainty if an in-flight value write loses acknowledgement. |

Keep an existing lease or the received descriptor's transfer pin alive
while attaching, reading or writing by token. A stopped operation can
retire its temporary fid after returning. The server applies only
complete replacement frames. Closing a writer does not roll back a
replacement already accepted.

Close leases with [`styxrpc_vfs_close`](vfs_close.md). Providers enable shared cells
with [`styxrpc_named_cells_enable`](named_cells_enable.md) before opening endpoints.
The [value frame validator](cell_value.md) checks the envelope and checksum of graph
frames. The receiving VM validates and decodes the graph body.

The wire extends [`/.channels`](named_channel.md) with operations 4 through 7.
Create takes a value frame and replies with a 24-byte token. Attach takes
that token and acknowledges at completion of the request. Get takes an empty
body and replies with the current value frame. Set takes a value frame and
acknowledges the replacement. Each cell uses a separate read/write Styx fid.
Send a reference as tag 8 followed by the 24-byte token.

In [Limbo](../../../disVM/module/styxnamed.m), `share(frame)` allocates a cell and
`adopt(frame)` attaches to a received descriptor. Use `cell.get()` and
`cell.set(value)` for the shared value frame. `cell.payload()` returns the descriptor
for forwarding and `cell.close()` releases the lease. The tagged frames can
carry scalars or graph snapshots while the Limbo handle has type `ref Cell`.
[Executable AS](../tests/flashvm/named_pair.as) · [Executable Limbo](../tests/flashvm/named_pair.b).
