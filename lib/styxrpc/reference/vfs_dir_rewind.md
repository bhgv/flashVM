# styxrpc_vfs_dir_rewind

[Русский](vfs_dir_rewind.ru.md) · [Directory cursors](vfs_directory.md) · [Header](../include/styxrpc/vfs_client.h) · [Implementation](../src/vfs_directory.c)

```c
#include <styxrpc/vfs_client.h>
StyxRpcIoOutcome styxrpc_vfs_dir_rewind(StyxRpcVfsDir *dir);
```

Link the hosted SDK. Pass a live `dir` that the caller owns. The function
discards buffered records and resets the byte offset, EOF and previous
iteration failure on that cursor. It returns `STYXRPC_IO_OK`, or
`STYXRPC_IO_NOT_OPEN` for null. It leaves the underlying Styx fid open
and performs no I/O. The next `styxrpc_vfs_dir_next` reads at offset zero
on that fid. This reports an I/O error if peer permissions or identity
prevent continuing. The call does not resolve a pathname or substitute
another directory.

Serialize calls on each cursor. Rewind is valid after reaching EOF or
after a directory read error. A failed transport still requires a new
session. For example, after a successful
`styxrpc_vfs_dir_rewind(dir)`, call `styxrpc_vfs_dir_next` until EOF
to read the current directory from the start. Use
[close](vfs_directory.md) to consume the cursor.
