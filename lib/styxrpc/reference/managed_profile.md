# styxrpc_managed_profile

[Русский](managed_profile.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/managed.h>
StyxRpcStatus styxrpc_managed_profile(const StyxRpcManagedExport *exported,
    void *bytes, size_t capacity, size_t *size);
```

Encode one shaped export into bytes of capacity bytes. On success size is the extent to append to .interface. The callback pointer stays local.

C99. Serialize operations on the same fid. `StyxRpc::rpc`.

[Example](../examples/adjust.c) · [Source](../src/managed.c)
