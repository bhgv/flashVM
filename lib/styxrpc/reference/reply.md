# styxrpc_reply

[Русский](reply.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/rpc.h>
StyxRpcStatus styxrpc_reply(uint32_t signature, const void *bytes, size_t size, int64_t *value);
```

Decode exactly size received bytes against signature. value is unchanged on INVALID or REMOTE_ERROR. Collect the entire reply before calling.

C99. The dispatcher serializes operations on each fid. `StyxRpc::rpc`.

[Example](../examples/client.c) · [Source](../src/rpc.c)
