# styxrpc_vfs_int_read

[Русский](vfs_int_read.ru.md) · [Source](../src/vfs_client.c)

```c
#include <styxrpc/vfs_client.h>
StyxRpcIoOutcome styxrpc_vfs_int_read(StyxRpcVfsFile *file, int32_t *value);
```

`StyxRpc::launch`

Read the complete native signed 32-bit variable from an opened `.var` file with one request at offset zero. `value` is caller-owned and changes only after the exact four-byte big-endian value is confirmed. A short or refused transfer gives `STYXRPC_IO_FAILED`; absent confirmation can give `STYXRPC_IO_UNCERTAIN`. Keep the fid open and serialize accesses.
