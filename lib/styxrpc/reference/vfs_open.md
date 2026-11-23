# styxrpc_vfs_open

[Русский](vfs_open.ru.md) · [Source](../src/vfs_client.c)

```c
#include <styxrpc/vfs_client.h>
StyxRpcIoOutcome styxrpc_vfs_open(struct StyxClient *session, const char *path,
    StyxRpcFileMode mode, StyxRpcVfsFile **file);
```

`StyxRpc::launch`

Use an already attached `session`, a nonempty [relative path](vfs_stat.md), and `STYXRPC_FILE_READ`, `STYXRPC_FILE_WRITE` or `STYXRPC_FILE_RDWR`. Opens one existing regular file. On `STYXRPC_IO_OK` the caller owns the fid in `file`. Admission failure returns `STYXRPC_IO_NOT_OPEN` and preserves the output. Serialize operations on the handle and consume it with [close](vfs_close.md).
