# styxrpc_read

[Русский](read.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/rpc.h>
StyxRpcStatus styxrpc_read(const StyxRpcCall *call, size_t offset, void *bytes, size_t capacity, size_t *size);
```

Copy up to capacity bytes of the immutable reply for call from byte offset to bytes. OK sets size to actual count, zero at EOF. MORE means the reply is pending. INVALID rejects bad parameters or a refused call.

C99. The dispatcher serializes operations on each fid. `StyxRpc::rpc`.

[Example](../examples/device.c) · [Source](../src/rpc.c)
