# styxrpc_vfs_file_stat

[Русский](vfs_file_stat.ru.md) · [Source](../src/vfs_client.c) · [styxrpc_vfs_file_stat_until](vfs_file_stat_until.md)

```c
#include <styxrpc/vfs_client.h>
StyxRpcIoOutcome styxrpc_vfs_file_stat(StyxRpcVfsFile *file, StyxRpcFileInfo *info);
```

`StyxRpc::launch`

Request the metadata of the exact held `file`. The caller owns `info`; success copies byte length, permission bits and directory flag. A missing handle gives `STYXRPC_IO_NOT_OPEN`. A failed stat gives `STYXRPC_IO_FAILED`. Errors preserve the output. Serialize with operations and close.
