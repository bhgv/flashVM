# Shared scalar cells on a named channel

[Русский](named_cell.ru.md) · [Header](../include/styxrpc/named_cell.h) · [Channels](named_channel.md) · [FlashVM](../../../flashVM/doc/STYX_RPC_NAMED_SEND.md)

A cell belongs to one attached registry. The issuer uses host entropy and a
serial to identify it. Each open cell fid holds a lease. A named channel also
holds the lease while the descriptor is pending and after receiving its
frame, up to the next request or clunk. The receiver opens its own lease before
starting another channel operation. Final release retires the token.

The cell uses the portable number, counted text, Boolean, null and undefined
frames. A write replaces one complete frame atomically. Successive reads and
writes are separate operations. Operations on one fid must be serialized.
The cell may change scalar kind on any assignment.

| C client call | Contract |
| --- | --- |
| [`styxrpc_cell_create`](cell_create.md) | Create a cell and receive its first lease and token. |
| [`styxrpc_cell_attach`](cell_attach.md) | Open a lease from a received token. |
| [`styxrpc_cell_get`](cell_get.md) | Read a complete scalar frame. |
| [`styxrpc_cell_set`](cell_set.md) | Replace the complete scalar frame. |

Close leases with [`styxrpc_vfs_close`](vfs_close.md). Providers enable shared cells
with [`styxrpc_named_cells_enable`](named_cells_enable.md) before opening endpoints.
The [frame validator](cell_scalar.md) is available in the platform-independent codec.

The wire extends [`/.channels`](named_channel.md) with operations 4 through 7.
Create takes a scalar frame and replies with a 24-byte token. Attach takes
that token and acknowledges at completion of the request. Get takes an empty
body and replies with the current scalar. Set takes a scalar frame and
acknowledges the replacement. Each cell uses a separate read/write Styx fid.
Send a reference as tag 8 followed by the 24-byte token.

In [Limbo](../../../disVM/module/styxnamed.m), `share(frame)` allocates a cell and
`adopt(frame)` attaches to a received descriptor. Use `cell.get()` and
`cell.set(value)` for the shared scalar. `cell.payload()` returns the descriptor
for forwarding and `cell.close()` releases the lease. The tagged frames can
carry different scalar kinds while the Limbo handle has type `ref Cell`.
[Executable AS](../tests/flashvm/named_pair.as) · [Executable Limbo](../tests/flashvm/named_pair.b).
