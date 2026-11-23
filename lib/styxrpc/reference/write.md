# styxrpc_write

[Русский](write.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/rpc.h>
StyxRpcStatus styxrpc_write(StyxRpcCall *call, size_t offset, const void *bytes, size_t size);
```

Stage size request bytes at the next contiguous byte offset on call. MORE acknowledges a partial frame. OK acknowledges completion; the callback runs synchronously once. INVALID rejects malformed frames and noncontiguous writes. Serialize operations on this call.

C99. The dispatcher serializes operations on each fid. `StyxRpc::rpc`.

[Example](../examples/device.c) · [Source](../src/rpc.c)
