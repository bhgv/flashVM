# Live references to mounted module variables

[Русский](module_refs.ru.md) · [Named channels](named_channel.md) · [FlashVM](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC_PEER_CHANNELS.md#pass-an-exported-variable)

A pair of applications can transfer a reference to a mounted Int32
variable on its existing named channel. The reference identifies one
exact export under `mounted/<image>/.var/<variable>` on the original
owner's VFS. Access follows the descriptor's mode and the current export's
permissions. Replacement or withdrawal retires the old variable's fids.

## Negotiate support

In a FlashVM pair, the forward owner creates the [scoped named service](named_peer_host_scoped.md)
with 16 fresh random bytes for this connection. The reverse owner uses
`STYXRPC_MODULE_REF_ATTACH` in the Tattach on the separate named-service
stream and [reads the scope](named_peer_read_ref_scope.md). The forward
application checks [the remote attach](named_peer_ref_scope.md). Send
tag 9 only after negotiating support. Ordinary VFS mounts continue to
serve each peer's live tree.

In a mounted DisVM and FlashVM pair, DisVM publishes the connection scope
read-only on its ordinary reverse mount. Its forward attach name
`STYXRPC_MODULE_REF_BOTH_ATTACH` advertises both origin roles. The
earlier scope-only profile supports references to the forward FlashVM.
The Dis application uses an [SDK reference port](module_ref_port.md)
through its Limbo adapter. For DisVM-owned exports that port resolves the
path and Qid previously issued by the ordinary reverse server.

## Transfer and resolve

On the importing side, call [identified open](vfs_int_open_identified.md)
on the peer's ordinary mount. Record its Qid, the peer's role, the same
relative path and the chosen access mode. [Encode](module_ref_encode.md)
this descriptor, prepend tag 9 and pass the frame to the named service.
Use [decode](module_ref_decode.md) with the negotiated scope before
resolving or forwarding it. Forward the decoded owner, mode, Qid and path
together on the same connection.

If the receiver owns the publication, it opens the
[local slot file](module_ref_open_local.md) on the slot owner thread and uses
[local reads](module_ref_read_local.md) and [writes](module_ref_write_local.md).
Close it with `styxrpc_file_close` before destroying the slot. The other
side [opens the remote fid](module_ref_open_remote.md) on the original owner's
ordinary mount and uses [read](vfs_int_read.md), [write](vfs_int_write.md)
and [close](vfs_close.md). Both opens require the same Qid.path. An application
can defer the remote open until first access so it can finish rendezvous
with a waiting owner before making VFS requests.

Use [cancellable open](module_ref_open_remote_until.md), [read](vfs_int_read_until.md)
and [write](vfs_int_write_until.md) when an SDK operation must respond to stop.
After interrupting a pending request, retire its fid and reopen the original
descriptor if needed. A replacement publication has its own Qid.
[Runnable FlashVM and DisVM example](../examples/disvm/INT32_REFERENCES.md).

A Limbo guest can cancel a wait on a FlashVM export through `Processctl.cancelwait`.
The shared [reference port](module_ref_port.md) checks the original Qid on the next access.

## Frame layout

The named frame begins with one byte containing 9
(`STYXRPC_NAMED_MODULE_REF`). The rest of the frame is the descriptor below.
The channel's existing frame length determines the path size.

| Payload offset | Bytes | Field |
| --- | --- | --- |
| 0 | 2 | ASCII `MV` |
| 2 | 1 | Version 1 |
| 3 | 1 | Bit 0 for write access, bit 1 for reverse owner. Other bits are zero. |
| 4 | 8 | Nonzero Qid.path, unsigned big-endian |
| 12 | 16 | Fresh scope bytes for this connection |
| 28 | Remainder | Canonical [relative export path](module_ref_path.md), without NUL |

The random scope identifies the pairing. The peers use the permissions
of their already authorized ordinary VFS. The
[wire and lifetime test](../tests/module_ref.c) checks the encoded bytes,
the scope, read-only access and replacement with a second image at the same path.
