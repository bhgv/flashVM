# styxrpc_named_peer_client

[Русский](named_peer_client.ru.md) · [Header](../include/styxrpc/named_peer.h#L32) · [Named host](named_peer_host.md)

```c
#include <styxrpc/named_peer.h>

struct StyxClient *styxrpc_named_peer_client(StyxRpcNamedPeer *peer);
```

Borrow the local Styx attachment from a live `peer` returned by
[named_peer_host](named_peer_host.md). NULL input returns NULL. This call
does no I/O. Use the result with [named channels](named_channel.md) and
[shared cells](named_cell.md), then release the opened fids. Ownership
stays with `peer`. End calls on the result before destroying `peer`.
A borrowed pointer says nothing about the current transport state. Each
subsequent operation reports its own I/O result. Available in the hosted
`StyxRpc::serve` target.
