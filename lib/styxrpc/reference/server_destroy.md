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
