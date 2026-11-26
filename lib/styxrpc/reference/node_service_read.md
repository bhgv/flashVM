# styxrpc_node_service_read

[Русский](node_service_read.ru.md) · [Declaration](../include/styxrpc/node_service.h#L33) · [Implementation](../src/node_service.c#L134) · [Wire protocol](node_exec.md)

```c
#include <styxrpc/node_service.h>
int styxrpc_node_service_read(StyxRpcNodeService *file, uint64_t offset,
    void *bytes, size_t size, size_t *used);
```

Link StyxRpc::rpc. Serialize operations on one fid on the transport owner.

Copy up to size confirmed reply bytes at the given offset into caller storage. Before successful completion returns 0. After completion returns 1 and sets used to the copied prefix length, or zero at EOF. Repeated and positioned reads use the same stored result.

```c
ok = styxrpc_node_service_read(fid, offset, bytes, size, &used);
```
