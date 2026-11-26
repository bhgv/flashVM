# Named rendezvous channels

[Русский](named_channel.ru.md) · [C declarations](../include/styxrpc/named_channel.h) · [Client](../src/named_channel_client.c) · [Registry](../src/named_channel.c) · [FlashVM](../../../flashVM/doc/STYX_RPC_NAMED_CHANNEL.md)

Link the hosted `StyxRpc::launch` target and use an already attached client.
`styxrpc_named_open(session, path)` returns one owned, read/write fid or NULL.
Pass a canonical absolute VFS path with fewer than 1024 bytes including the
leading slash. Release the fid with `styxrpc_vfs_close`.
`styxrpc_named_read` blocks until a match and transfers a malloc-owned frame.
`styxrpc_named_write` borrows a nonempty frame until the other side accepts it.
Both return a `StyxRpcIoOutcome`. Serialize operations on each individual fid.
Different fids can operate in separate OS threads. Pending waits poll in bounded
requests without holding the transport lock between polls. A broken session
ends the wait. Close the endpoint on a failed operation.

Providers can link `StyxRpc::serve` and use `styxrpc_named_registry` with
optional namespace claim, visibility and release hooks. For each control fid
allocate `styxrpc_named_endpoint`, dispatch offsets through
`styxrpc_named_submit` and `styxrpc_named_reply`, and call
`styxrpc_named_retire` on clunk or connection loss. The registry pairs the oldest
opposite operation without buffering completed writes. Destroy the registry
after its final endpoint. Callbacks must keep state alive and must not reenter
the registry.

The reserved `/.channels` service accepts requests over an ordinary read/write
fid. Write the operation byte followed by a four-byte big-endian body length
and the body. Operation 1 enrolls an absolute VFS path, 2 posts a read and 3 posts
a write. Each operation begins at offset zero. For read and write, poll the
reply at offset zero. A zero-byte read means pending. Replies start with a
four-byte big-endian payload size. A size of zero acknowledges a matched write.
A read reply carries the writer's frame. Fragments are transferred at consecutive
offsets. Each value carries a mandatory type byte. The header defines the
scalar, snapshot and issuer-descriptor tags.

DisVM ships [Styxnamed](../../../disVM/module/styxnamed.m). Load
`styxnamed.dis`, call `init()`, and `open("/jobs")` in the paired namespace.
The Limbo methods operate on tagged byte arrays. Scalars and descriptors have
portable formats. FlashVM graph payloads can be forwarded intact from Limbo.
See the compiled [Limbo](../tests/flashvm/named_pair.b) and
[ActionScript](../tests/flashvm/named_pair.as) acceptance sources.
