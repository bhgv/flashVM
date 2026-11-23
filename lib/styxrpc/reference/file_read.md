# styxrpc_file_read

[Русский](file_read.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/vfs.h>
StyxRpcStatus styxrpc_file_read(StyxRpcFile *file, uint64_t offset, void *bytes, size_t capacity, size_t *size);
```

Read from file at byte offset into caller-owned bytes, up to capacity bytes. OK reports the confirmed size, zero at EOF. INVALID rejects missing read access, an invalid buffer or callback failure.

C99. The dispatcher serializes operations on each fid. `StyxRpc::rpc`.

[Example](../examples/device.c) · [Source](../src/vfs.c)
