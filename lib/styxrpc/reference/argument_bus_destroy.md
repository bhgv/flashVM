# styxrpc_argument_bus_destroy

[Русский](argument_bus_destroy.ru.md) · [Declaration](../include/styxrpc/channel_arguments.h#L31) · [Implementation](../src/channel_arguments.c) · [Scope guide](channel_argument_scopes.md)

Link `StyxRpc::launch`.

```c
#include <styxrpc/channel_arguments.h>
void styxrpc_argument_bus_destroy(StyxRpcArgumentBus *bus);
```

Release the owned `bus` after the reverse server, every operation fid and all invocation scopes have closed. Serialize destruction with scope management. A null bus is accepted. After destruction, descriptors borrowed from it are invalid.

[C regression](../tests/channel_arguments.c)
