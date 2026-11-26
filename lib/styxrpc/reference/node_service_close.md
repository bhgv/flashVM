# styxrpc_node_service_close

[Русский](node_service_close.ru.md) · [Declaration](../include/styxrpc/node_service.h#L39) · [Implementation](../src/node_service.c#L153) · [Wire protocol](node_exec.md)

```c
#include <styxrpc/node_service.h>
void styxrpc_node_service_close(StyxRpcNodeService *file);
```

Link StyxRpc::rpc. Serialize operations on one fid on the transport owner.

Consume one owned cursor when it has no concurrent I/O or callback. Free its staged bytes and result. NULL is harmless. Closing an incomplete request does not invoke its callback. Context remains owned by the host.

```c
styxrpc_node_service_close(fid);
```
