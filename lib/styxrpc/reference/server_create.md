# styxrpc_server_create

[Русский](server_create.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/serve.h>
StyxRpcServer *styxrpc_server_create(struct RemotePhy *phy, const StyxRpcTree *tree, void *context);
```

Consume the delegated stream and borrow tree/context through destroy. Supply root ID zero and globally stable IDs within this connection. The stat callback returns the current name, parent, byte length and directory revision. The enumerate callback receives the directory and a zero-based index and returns one immediate child or NULL. Increase the revision when changing membership or order. Only the other-class mode bits grant read, write and search in this adapter. The stream must already identify the authorized peer. Returns an owned server or NULL.

Hosted Linux64. `StyxRpc::serve`.

[Example](../tests/directory.c) · [Source](../src/serve.c)
