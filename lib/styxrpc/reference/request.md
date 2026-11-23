# styxrpc_request

[Русский](request.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/rpc.h>
StyxRpcStatus styxrpc_request(uint32_t signature, int64_t argument, void *bytes, size_t capacity, size_t *size);
```

Encode one signed argument with the compiled signature into bytes. capacity and size are byte counts. The caller owns both buffers. On failure size is zero.

C99. The dispatcher serializes operations on each fid. `StyxRpc::rpc`.

[Example](../examples/client.c) · [Source](../src/rpc.c)
