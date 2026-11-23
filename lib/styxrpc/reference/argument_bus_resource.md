# styxrpc_argument_bus_resource

[Русский](argument_bus_resource.ru.md) · [Declaration](../include/styxrpc/channel_arguments.h#L26) · [Implementation](../src/channel_arguments.c) · [Scope guide](channel_argument_scopes.md)

Link `StyxRpc::launch`.

```c
#include <styxrpc/channel_arguments.h>
StyxRpcResource styxrpc_argument_bus_resource(StyxRpcArgumentBus *bus);
```

Return a descriptor whose callbacks borrow `bus`. Register it on that PID's [reverse .unibus](program_unibus.md) before serving. The reverse dispatcher uses these callbacks independently of the forward owner. Opens require read/write access and an active scope. Operations reply from the confirmed request length. Keep the bus alive through the final resource close.

[C regression](../tests/channel_arguments.c)
