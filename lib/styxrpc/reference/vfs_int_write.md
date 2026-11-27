# styxrpc_vfs_int_write

[Русский](vfs_int_write.ru.md) · [Source](../src/vfs_int.c#L134)

```c
#include <styxrpc/vfs_client.h>
StyxRpcIoOutcome styxrpc_vfs_int_write(StyxRpcVfsFile *file, int32_t value);
```

`StyxRpc::launch`

Replace a native signed 32-bit variable on an opened writable `.var` file. Send one four-byte big-endian `value` at offset zero, and require confirmation of all bytes. A short or refused transfer gives `STYXRPC_IO_FAILED`. If the reply is lost, report `STYXRPC_IO_UNCERTAIN` without replay. Keep the fid open and serialize accesses.

[Cancellable form](vfs_int_write_until.md).
