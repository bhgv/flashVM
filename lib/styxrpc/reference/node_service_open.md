# styxrpc_node_service_open

[Русский](node_service_open.ru.md) · [Declaration](../include/styxrpc/node_service.h#L20) · [Implementation](../src/node_service.c#L62) · [Wire protocol](node_exec.md)

```c
#include <styxrpc/node_service.h>
StyxRpcNodeService *styxrpc_node_service_open(StyxRpcNodeInvoke invoke, void *context);
```

Link StyxRpc::rpc. Serialize operations on one fid on the transport owner.

Allocate one private execute cursor before accepting wire data. The caller owns the result and closes it after clunk. The cursor borrows invoke and context. The callback receives a NUL-terminated relative spelling, a mutable argument buffer, its size and an output count. Return 1 from the callback with a count up to size, or 0 for refusal. A missing callback or allocation failure returns NULL.

```c
StyxRpcNodeService *fid = styxrpc_node_service_open(invoke, context);
```
