# styxrpc_managed_read

[Русский](managed_read.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/managed.h>
StyxRpcStatus styxrpc_managed_read(const StyxRpcManagedCall *call, size_t offset, void *bytes,
    size_t capacity, size_t *size);
```

Copy up to capacity reply bytes at byte offset. MORE means pending, OK and zero bytes means EOF. The caller owns bytes and size.

C99. Serialize operations on the same fid. `StyxRpc::rpc`.

[Example](../examples/adjust.c) · [Source](../src/managed.c)
