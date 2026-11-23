# styxrpc_file_open

[Русский](file_open.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/vfs.h>
StyxRpcStatus styxrpc_file_open(StyxRpcFile *file, const StyxRpcVfs *ops, void *context, uint64_t node, unsigned access);
```

Bind zero-initialized caller-owned file to node using ops and context. access uses STYXRPC_READ and STYXRPC_WRITE. OK keeps the callback cookie until close. INVALID means the open was denied or malformed. The dispatcher checks the node permissions before opening.

C99. The dispatcher serializes operations on each fid. `StyxRpc::rpc`.

[Example](../examples/device.c) · [Source](../src/vfs.c)
