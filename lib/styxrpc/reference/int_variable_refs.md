# Transferring Int32 references

[Русский](int_variable_refs.ru.md) · [API](../include/styxrpc/variable_ref.h) · [AS references](../../../flashVM/doc/AS_VARIABLE_REFS.md)

The issuer publishes a live cell with `styxrpc_int_ref_grant` on its program
server. The callbacks read and write the owner's value. The grant selects
read-only or read/write access. Keep callback state alive through revocation
or server teardown.

`styxrpc_int_ref_encode` and `styxrpc_int_ref_decode` use an eight-byte
payload. It contains the `VR` prefix, version 1, a write flag of 0 or 1 and a
network-order PID. Carry it as bytes or in a registered UniBus message.
Keep the issuing session known at the receiver. The same PID on another
session can name a different variable. Forward the descriptor on the same
issuer session.

`styxrpc_int_ref_open` checks the PID generation in the Qid and returns an
owned fid. Release it with `styxrpc_vfs_close`. Read or write through
`styxrpc_vfs_int_read` and `styxrpc_vfs_int_write`. Revocation via
`styxrpc_int_ref_revoke` invalidates held fids. Issue and revoke outside server
callbacks. Slot reuse assigns a fresh PID.

FlashVM's `styxrpc.acceptIntRef(bytes)` returns a reference that can bind a new
name. Reads and writes via that name access the owner.
`styxrpc.intRefBytes(ref alias)` returns the original bytes for forwarding
through a byte channel on the same session. Limbo's
`Styxintref->accept(root, bytes)` returns a `ref Cell` with `read()`,
`write(value)` and `payload()`. Its adapter serializes cursor positioning
and I/O on the held fid. Pass the issuer's mount as `root`.

In AS, `var refs = styxrpc.intRefChannel(wire)` wraps a dedicated byte
carrier with `send(bytes)` and `recv()`. Then `refs.send(ref alias)`
encodes the issued reference and `var next = refs.recv()` binds the new
name. If the carrier returns undefined, the wrapper preserves it. The
caller selects the carrier for the same issuer session used to open cells.

In Limbo, `refs->channel(root, wire)` returns a port with `send(cell)` and
`recv()`. Send requires the same issuer mount name. Receive waits using the
ordinary Limbo channel operation.

Both wrappers forward references the issuer has already published.
