# styxrpc_vfs_create

[Русский](vfs_create.ru.md) · [Header](../include/styxrpc/vfs_client.h) · [Source](../src/vfs_create.c) · [Open an existing file](vfs_open.md) · [styxrpc_vfs_create_until](vfs_create_until.md)

```c
#include <styxrpc/vfs_client.h>
StyxRpcIoOutcome styxrpc_vfs_create(struct StyxClient *session, const char *path,
    StyxRpcVfsFile **file);
```

Link `StyxRpc::launch`. Supply an attached session, a nonempty ordinary
[relative path](vfs_stat.md) and the address of a file pointer. The parent
directory must already exist. The library sends one write-only Styx
create for a regular file with requested mode 0666. The provider applies
its permissions and refuses occupied names.

On `STYXRPC_IO_OK`, the caller owns the new write-only fid. Serialize
its use and release it with [close](vfs_close.md). On failure `file` is
preserved. `STYXRPC_IO_NOT_OPEN` means creation was rejected before the
request. `STYXRPC_IO_FAILED` means it returned without an open handle.
`STYXRPC_IO_UNCERTAIN` means delivery was ambiguous. Inspect the directory
before retrying. The call can wait for Styx I/O up to the session deadline.
