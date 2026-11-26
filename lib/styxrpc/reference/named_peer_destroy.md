# styxrpc_named_peer_destroy

[Русский](named_peer_destroy.ru.md) · [Header](../include/styxrpc/named_peer.h#L39) · [Named host](named_peer_host.md)

```c
#include <styxrpc/named_peer.h>

void styxrpc_named_peer_destroy(StyxRpcNamedPeer *peer);
```

Consume an owned `peer` from [named_peer_host](named_peer_host.md). NULL is
valid. The host stops both dispatchers, wakes the local client, joins
the native threads and releases the local mount. The call can wait for
server work to retire. Call on the host after ending operations on the
borrowed client. Keep the registry and its callback state alive until
the call returns. Available in the hosted `StyxRpc::serve` target.
