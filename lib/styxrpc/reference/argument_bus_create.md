# styxrpc_argument_bus_create

[Русский](argument_bus_create.ru.md) · [Declaration](../include/styxrpc/channel_arguments.h#L21) · [Implementation](../src/channel_arguments.c) · [Scope guide](channel_argument_scopes.md)

Link `StyxRpc::launch`.

```c
#include <styxrpc/channel_arguments.h>
StyxRpcArgumentBus *styxrpc_argument_bus_create(uint32_t pid);
```

Allocate one caller-owned reverse argument bus for the nonzero caller `pid`. Return its owned handle or `NULL` for an invalid PID or allocation or lock failure. Keep one bus on the selected reverse connection. Publish its [resource](argument_bus_resource.md) before server dispatch starts. Close it with [destroy](argument_bus_destroy.md) after shutdown.

[C regression](../tests/channel_arguments.c)
