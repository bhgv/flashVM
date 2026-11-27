# styxrpc_server_destroy

[Русский](server_destroy.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/serve.h>
void styxrpc_server_destroy(StyxRpcServer *server);
```

Consume the server and retire every opened fid cookie on its owning thread, including after a disconnect. NULL is allowed. Keep application state alive until this returns.

Hosted Linux64. `StyxRpc::serve`.

[Example](../tests/directory.c) · [Source](../src/serve.c)

When [deferred file I/O](server_enable_deferred_io.md) is enabled, destruction
signals the callbacks and waits for them to return. The owner closes held
file cookies after the corresponding worker has exited.
