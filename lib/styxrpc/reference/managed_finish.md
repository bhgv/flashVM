# styxrpc_managed_finish

[Русский](managed_finish.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/managed.h>
StyxRpcStatus styxrpc_managed_finish(StyxRpcManagedCall *call, const StyxRpcOutput *output, void *scratch, size_t capacity);
```

Complete a pending fid from disjoint scratch of capacity bytes. NULL output publishes a remote error. Bad output becomes a remote error when its S-record fits. Calling twice or after close returns INVALID.

C99. Serialize operations on the same fid. `StyxRpc::rpc`.

[Example](../examples/../tests/managed.c) · [Source](../src/managed.c)
