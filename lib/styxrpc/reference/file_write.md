# styxrpc_file_write

[Русский](file_write.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/vfs.h>
StyxRpcStatus styxrpc_file_write(StyxRpcFile *file, uint64_t offset, const void *bytes, size_t count, size_t *size);
```

Pass count bytes to file at byte offset. OK reports the acknowledged size, which may be less than count. INVALID rejects missing write access, bad arguments or a callback failure.

C99. The dispatcher serializes operations on each fid. `StyxRpc::rpc`.

[Example](../examples/device.c) · [Source](../src/vfs.c)
