# styxrpc_argument_scope_open

[Русский](argument_scope_open.ru.md) · [Declaration](../include/styxrpc/channel_arguments.h#L36) · [Implementation](../src/channel_arguments.c) · [Scope guide](channel_argument_scopes.md)

Link `StyxRpc::launch`.

```c
#include <styxrpc/channel_arguments.h>
StyxRpcArgumentScope *styxrpc_argument_scope_open(StyxRpcArgumentBus *bus, uint32_t pid);
```

Reserve a staging scope on `bus`, for its exact nonzero `pid`. Return an owned scope, or null when the bus or PID is wrong, another scope occupies the bus, or allocation fails. No key is visible on the reverse tree while staging. Consume the scope with [invoke](argument_scope_invoke.md) or [close](argument_scope_close.md).

[C regression](../tests/channel_arguments.c)
