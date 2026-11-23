# styxrpc_channel_read_routed

[Русский](channel_read_routed.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel_route.h>
StyxRpcStatus styxrpc_channel_read_routed(const StyxRpcChannelGrant *grants, size_t grant_count,
    const StyxRpcChannelPolicy *policy, const void *bytes, size_t size, StyxRpcSelection *operation);
```

Decode one complete channel request at `bytes` with byte `size`. Each guard
must use an exact registered key from the borrowed `grants` table of
`grant_count` entries. Optional `policy` authorizes each distinct channel
address contained in a typed value. A null policy selects ordinary call-local
decoding. A nonnull policy applies to all nested addresses including CURRENT.

The `allow` callback returns 1 to permit the exact declared address, key,
capacity and type under the current application authority. It must be
side-effect-free. The codec can call it again during scheduling and reply
validation. Supply the same borrowed policy in `StyxRpcChannelFid.policy`
for the fid adapter or in `StyxRpcSelection.policy` for requests and replies
constructed by hand. The application serializes callbacks and keeps the
policy, its context and request buffer alive through completion or clunk.

`STYXRPC_OK` publishes a selection borrowing the guard data and policy.
`STYXRPC_INVALID` leaves `operation` unchanged. Malformed records, unknown
keys and unauthorized nested addresses all fail. The function is synchronous
and performs no transport I/O. Link `StyxRpc::rpc`.

[styxrpc_channel_fid_write](channel_fid_write.md) · [Example](../examples/disvm/routed_client.c) · [Source](../src/channel.c)
