# styxrpc_node_service_write

[Русский](node_service_write.ru.md) · [Declaration](../include/styxrpc/node_service.h#L27) · [Implementation](../src/node_service.c#L76) · [Wire protocol](node_exec.md)

```c
#include <styxrpc/node_service.h>
int styxrpc_node_service_write(StyxRpcNodeService *file, uint64_t offset,
    const void *input, size_t size, size_t *used);
```

Link StyxRpc::rpc. Serialize operations on one fid on the transport owner.

Supply a nonempty borrowed fragment at the next expected byte offset, starting at zero. On success returns 1 and sets used to the fragment size. The final fragment invokes the callback synchronously once. Returns 0 on malformed input or handler refusal. Close the cursor after failure. Keep the cursor, callback context and input live until the call returns.

```c
ok = styxrpc_node_service_write(fid, offset, bytes, size, &used);
```
