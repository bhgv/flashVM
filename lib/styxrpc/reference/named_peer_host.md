# styxrpc_named_peer_host

[Русский](named_peer_host.ru.md) · [Header](../include/styxrpc/named_peer.h#L30) · [Source](../src/named_peer.c#L230) · [Named protocol](named_channel.md)

```c
#include <styxrpc/named_peer.h>

StyxRpcNamedPeer *styxrpc_named_peer_host(struct RemotePhy *lane,
    StyxRpcNamedRegistry *registry, unsigned msize, unsigned timeout,
    unsigned tags);
```

Link `StyxRpc::serve` in a hosted Linux64 build. The function consumes
`lane` on success or failure. Supply the connected, ordered byte stream
already delegated to the named service. `registry` remains borrowed until
both dispatchers have retired. Configure its admission callbacks and
optional [cell entropy](named_cell.md) before calling.

The host runs one native Styx server on `lane` and one on a private local
socket pair. Each server opens endpoints in the same registry. The function
waits for the local mount to attach and returns an owned handle. It returns
NULL on invalid input, socket, thread or mount failure and closes all
partially created resources. `msize` is the requested local Styx message
size in bytes. `timeout` is its request budget in milliseconds. `tags`
bounds the number of outstanding native requests. The remote client
negotiates its own message size.

In a P9MX pair, exchange `STYXRPC_NAMED_PEER_ATTACH` on the ordinary
attachment before taking `STYXRPC_NAMED_PEER_LANE`. Mount the other end with
the normal Styx client and use the [named wire operations](named_channel.md).
For local calls, borrow [the local client](named_peer_client.md).
Keep namespace callbacks thread-safe and short. They run on the service
owners while a user of the local client can be blocked. Closing either
stream causes the other side to stop. Use [destroy](named_peer_destroy.md)
from the host thread after its users have stopped, then release the registry.

The [hosted acceptance](../tests/named_peer.c) shows startup, access through
both mounts and cancellation when the remote client shuts down.
