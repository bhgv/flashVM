# styxrpc_open

[Русский](open.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/rpc.h>
StyxRpcStatus styxrpc_open(StyxRpcCall *call, const StyxRpcExport *exported);
```

Initialize caller-owned call storage for one opened function fid. Keep exported alive and immutable through close. INVALID rejects a missing callback, unsupported signature or invalid name.

C99. The dispatcher serializes operations on each fid. `StyxRpc::rpc`.

[Example](../examples/device.c) · [Source](../src/rpc.c)
