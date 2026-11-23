# styxrpc_profile

[Русский](profile.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/rpc.h>
StyxRpcStatus styxrpc_profile(const StyxRpcExport *exports, size_t count, void *bytes, size_t capacity, size_t *size);
```

Encode count distinct function descriptors from exports in caller buffer bytes with capacity bytes. On INVALID size is zero. Publish exactly size bytes as .interface.

C99. The dispatcher serializes operations on each fid. `StyxRpc::rpc`.

[Example](../examples/device.c) · [Source](../src/rpc.c)
