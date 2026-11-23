# styxrpc_managed_write

[Русский](managed_write.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/managed.h>
StyxRpcStatus styxrpc_managed_write(StyxRpcManagedCall *call, size_t offset, const void *bytes, size_t size,
    void *scratch, size_t capacity);
```

Stage size bytes at the next contiguous byte offset. Invalid framing refuses the fid. A complete checked request invokes begin once. MORE means staged or accepted for deferred completion. Keep scratch disjoint from input, output and fid buffers.

C99. Serialize operations on the same fid. `StyxRpc::rpc`.

[Example](../examples/adjust.c) · [Source](../src/managed.c)
