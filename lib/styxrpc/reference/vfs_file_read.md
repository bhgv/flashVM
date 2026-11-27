# styxrpc_vfs_file_read

[Русский](vfs_file_read.ru.md) · [Source](../src/vfs_client.c)

```c
#include <styxrpc/vfs_client.h>
StyxRpcIoOutcome styxrpc_vfs_file_read(StyxRpcVfsFile *file, uint64_t offset,
    void *bytes, size_t size, size_t *used);
```

`StyxRpc::launch`

Read up to `size` bytes on the already opened `file` from `offset`. The caller supplies writable `bytes` and `used`, and keeps its own cursor. Apply [VFS bounds and outcomes](vfs_read_at.md). Only confirmed bytes contribute to `used`. A successful short result includes EOF. Serialize with other operations on this fid.

Use [styxrpc_vfs_file_read_until](vfs_file_read_until.md) when a caller
needs to stop while a data reply is pending.
