# styxrpc_channel_export_routes

[Русский](channel_export_routes.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel_route.h>
StyxRpcStatus styxrpc_channel_export_routes(const char *shape, const void *bytes, size_t size,
    StyxRpcChannelRoutes *routes);
```

Collect module channel addresses from a confirmed function result. Supply the
declared result value-list `shape`, the complete encoded result field `bytes`
(tag 90, within the result/update pair) and its exact byte `size`. Include the
outer value-list parentheses. A tuple containing a channel and an integer uses
`((c[i]i))`.

The initialized output on `STYXRPC_OK` contains up to eight distinct module
routes with result-only channel leaves (tag 31). The function checks the whole
value graph against the supplied schema. It coalesces consistent aliases and
rejects malformed or conflicting leaves. `STYXRPC_INVALID` leaves `routes`
untouched. Obtain this field from the successful reply on the same function fid
and keep that fid open while using its exported handles. The output stores
wire addresses and does no I/O or allocation. Link `StyxRpc::rpc`.

[styxrpc_channel_element_routes](channel_element_routes.md) · [Example](../examples/disvm/routed_client.c) · [Source](../src/channel_route.c)
