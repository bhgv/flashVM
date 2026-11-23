# styxrpc_channel_route_exchange

[Русский](channel_route_exchange.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel_route.h>
StyxRpcStatus styxrpc_channel_route_exchange(struct StyxClient *session, const char *module_bus,
    const StyxRpcChannelRoute *route, const StyxRpcSelection *selection, unsigned timeout_ms,
    void *scratch, size_t scratch_size, void *request, size_t request_capacity,
    void *reply, size_t reply_capacity, StyxRpcChannelResult *result);
```

Perform one synchronous send, receive or guard selection for the channel at
the delegated `route`. Use the exact connected `session` from which the
capability was received. For PID, pass null `module_bus`. The adapter opens
`.prog/<pid>/.unibus`. For MODULE, pass the relative path to the exact
module's `.unibus`. The adapter writes the token separately at offset zero,
writes the operation and reads its result from offset zero. CURRENT routes
use the containing bus and are handled by the channel codec there.

All guards in `selection` use `route->channel.key`. Supply nonoverlapping
buffers in bytes. `scratch` and `scratch_size` cover any ALT wrapper
assembly. `request` and `request_capacity` hold the encoded request.
`reply` and `reply_capacity` hold the complete framed reply. The result
borrows its typed payload until that reply buffer is reused. Keep the
session, authorization and borrowed data active through the operation.

`timeout_ms` is 1 through 60000 and is checked between synchronous Styx
transactions. The temporary operation fid is clunked on all paths. The
result is published only on `STYXRPC_OK`. `STYXRPC_MORE` means timeout.
`STYXRPC_REMOTE_ERROR` means an acknowledged channel error.
`STYXRPC_INVALID` covers invalid arguments, transport or authentication
failure, revoked routes and invalid frames. A timed-out operation may already
have committed. Never replay it automatically. Link `StyxRpc::launch`.

[styxrpc_channel_exchange](channel_exchange.md) · [Example](../examples/disvm/routed_client.c) · [Source](../src/channel_exchange.c)
