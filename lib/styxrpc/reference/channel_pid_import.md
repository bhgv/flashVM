# styxrpc_channel_pid_import

[Русский](channel_pid_import.ru.md) · [Declaration](../include/styxrpc/channel_route.h#L104) · [Implementation](../src/channel_pid.c#L72)

```c
#include <styxrpc/channel_route.h>
StyxRpcStatus styxrpc_channel_pid_import(struct StyxClient *reverse, const char *shape,
    const void *bytes, size_t size, StyxRpcChannelRoutes *routes);
```

Link `StyxRpc::launch`.

On the owner, import a complete typed element accepted for a hosted result. `reverse` is the borrowed Styx mount from the same caller. `shape` is a NUL-terminated parenthesized one-value schema containing channels. Supply the frame in `bytes` and its byte length in `size` up to `STYXRPC_MANAGED_LIMIT`. Channel leaves can be nil or native PID grants for channels with by-value elements. Initialize `routes` to zero and reuse it only for this result and mount.

The SDK stages distinct PIDs with stable indices, preserving exact aliases and rejecting conflicting identities, schemas or capacity flags. For each received PID it reads the exact decimal identity and checks `.unibus` under the same PID directory. These checks use the mounted client's timeout for each Styx operation. The caller provides the association between the message and the result. The PID is scoped to a native attachment whose grant IDs stay unique.

`STYXRPC_OK` commits the table. `STYXRPC_INVALID` preserves it on invalid input, missing resources, transport failure, or exhausted slots. Use [routed exchange](channel_route_exchange.md) for later operations on an admitted PID. The peer can revoke it after admission. [Socket test](../tests/channel_socket.c)
