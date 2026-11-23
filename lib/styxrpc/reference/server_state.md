# styxrpc_server_state

[Русский](server_state.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/serve.h>
int styxrpc_server_state(StyxRpcServer *server, StyxRpcServerState *state);
```

Read a snapshot of current clients, queued reply bytes and free reply slots from the existing session. No transport I/O and no counters added to the request path. Return 1 on success and 0 for NULL arguments. The owner may call between steps or in a callback.

Hosted Linux64. `StyxRpc::serve`.

[Example](../tests/directory.c) · [Source](../src/serve.c)
