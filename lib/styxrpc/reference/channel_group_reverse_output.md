# styxrpc_channel_group_reverse_output

[Русский](channel_group_reverse_output.ru.md) · [Declaration](../include/styxrpc/channel_group.h) · [Implementation](../src/channel_group.c) · [Wire adapter](../src/channel_group_wire.c)

Include `<styxrpc/channel_group.h>` and link `StyxRpc::serve`.

```c
StyxRpcStatus styxrpc_channel_group_reverse_output(const StyxRpcChannelQueue *queues,
    size_t count, size_t root, StyxRpcChannelOutput *output);
```

Accepts the same queue descriptors, 1–8 queue limit, zero-based root index and caller-owned output as [channel_group_output](channel_group_output.md). On success the output owns a copied group with its root key, capacity flag and release callback. `STYXRPC_INVALID` leaves the output unchanged for invalid input, invalid nested frames or allocation failure.

Register `output->resource` through [styxrpc_program_unibus_grant](program_unibus_grant.md) on the authorized reverse dispatcher. Advertise the returned reverse PID and the root key and type in a `STYXRPC_ROUTE_PID` value. Reads are relative to the confirmed request extent, with no leading module token. Serialize the resource on one dispatcher. [Revoke the publication](program_unibus_revoke.md) or destroy the server and close its fids before calling the resource's release function. The C host controls that publication lifetime.

[Standalone test](../tests/channel_group.c)
