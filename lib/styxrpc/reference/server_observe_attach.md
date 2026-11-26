# styxrpc_server_observe_attach

[Русский](server_observe_attach.ru.md) · [Header](../include/styxrpc/serve.h#L54) · [Implementation](../src/serve_attach.inc) · [Server step](server_step_for.md)

```c
#include <styxrpc/serve.h>

void styxrpc_server_observe_attach(StyxRpcServer *server,
    StyxRpcAttachNotice notice, void *state);
```

Install an observer on the server's owning thread outside a server step.
`server` is the live dispatcher. NULL is a no-op. `notice` receives
`state` and the borrowed NUL-terminated attach name during dispatch,
before the attach reply. The observer has no return value and cannot grant
permissions. Keep `state` alive until replacing or clearing the observer.
A NULL `notice` removes it. Copy the name if it is needed after the call.
The observer runs synchronously on the owner and must return without
reentering the server. The setter does no I/O. Link `StyxRpc::serve`.

The [named service bootstrap](named_peer_host.md) uses the name as a
protocol feature indication on an already delegated connection.
