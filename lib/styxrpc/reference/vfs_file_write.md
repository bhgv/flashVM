# styxrpc_vfs_file_write

[Русский](vfs_file_write.ru.md) · [Source](../src/vfs_client.c)

```c
#include <styxrpc/vfs_client.h>
StyxRpcIoOutcome styxrpc_vfs_file_write(StyxRpcVfsFile *file, uint64_t offset,
    const void *bytes, size_t size, size_t *used);
```

`StyxRpc::launch`

Write up to `size` bytes on the already opened `file` at `offset`. The caller owns the source `bytes`, required count `used` and its cursor. Apply [VFS bounds and outcomes](vfs_write_at.md). The SDK advances only by confirmed bytes and stops on failure or zero progress. An uncertain fragment is never replayed. Serialize with other operations on this fid.
