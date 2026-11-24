# styxrpc_server_step

[Русский](server_step.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/serve.h>
int styxrpc_server_step(StyxRpcServer *server);
```

Wait for a server event and dispatch it on the owning thread. Return 1 if the server can continue, 0 on error or disconnect. All application callbacks execute on that thread. Do not call step concurrently or recursively on the same server. Directory offsets count encoded bytes and must resume at the prior boundary. If membership changed, rereopen the directory and start at zero.

Hosted Linux64. `StyxRpc::serve`.

[Example](../tests/directory.c) · [Source](../src/serve.c)

[Serve waiting for the next deadline](server_step_for.md).
