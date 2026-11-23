# styxrpc_argument_scope_activate

[Русский](argument_scope_activate.ru.md) · [Declaration](../include/styxrpc/channel_arguments.h#L47) · [Implementation](../src/channel_arguments.c) · [Scope guide](channel_argument_scopes.md)

Link `StyxRpc::launch`.

```c
#include <styxrpc/channel_arguments.h>
int styxrpc_argument_scope_activate(StyxRpcArgumentScope *scope);
```

Use this with a custom forward transport that has already verified the exact scoped offer and is ready to write. `scope` is one staging scope. On success return 1 and publish its entire set of keys. Invalid state or queue metadata returns 0. Keep the queue arena unchanged by local users until [close](argument_scope_close.md). The standard [invoke](argument_scope_invoke.md) performs activation internally.

[C regression](../tests/channel_arguments.c)
