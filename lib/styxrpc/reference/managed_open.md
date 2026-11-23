# styxrpc_managed_open

[Русский](managed_open.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/managed.h>
StyxRpcStatus styxrpc_managed_open(StyxRpcManagedCall *call, const StyxRpcManagedExport *exported,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity);
```

Initialize fresh or closed call state using two disjoint, application-owned buffers. Each byte capacity is in 1..65536. Keep the descriptor and storage alive until close.

C99. Serialize operations on the same fid. `StyxRpc::rpc`.

[Example](../examples/adjust.c) · [Source](../src/managed.c)
