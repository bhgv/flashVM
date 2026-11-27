# styxrpc_vfs_file_stat_until

[Русский](vfs_file_stat_until.ru.md) · [Original API](vfs_file_stat.md) · [Shared contract](../VFS_CONTROL.md)

## Syntax

```c
#include <styxrpc/vfs_client.h>

StyxRpcIoOutcome styxrpc_vfs_file_stat_until(StyxRpcVfsFile *file, StyxRpcFileInfo *info,
    StyxRpcVfsStop stop, void *state);
```

Hosted C99, target `StyxRpc::launch`.

Send one Tstat on an opened fid. The same fid remains borrowed even when the path was renamed or the stat wait was cancelled.

## Parameters

| Parameter | Meaning |
| --- | --- |
| `file` | Borrowed opened file. Keep its fid valid and serialize operations on it. |
| `info` | Writable metadata output. Only success replaces its length in bytes, mode and directory flag. |
| `stop` | Optional short native callback on the calling thread. Return nonzero to stop. NULL keeps ordinary synchronous behavior. |
| `state` | Opaque stop argument borrowed until return. |

## Result

Statuses below use the `STYXRPC_IO_` prefix. `OK` confirms the result. `CANCELLED` reports a stop requested by the callback. On a multiplexed client, a sent request is flushed when cancellation wins. Serial transports sample the hook before an exchange. An original acknowledgement determines the result when it wins.

`FAILED` covers a missing or invalid stat reply. Cancellation leaves the cursor and metadata untouched.

## Example

Use an attached session or previously opened descriptor and a stop callback that reads your caller’s stop flag.

```c
StyxRpcFileInfo info;
StyxRpcIoOutcome status = styxrpc_vfs_file_stat_until(file, &info, stop, state);
```

The [wire test](../tests/vfs_control_wire.c) exercises this entry point. Run `ctest --test-dir build-styxrpc -R styxrpc-vfs-control-wire --output-on-failure` to check replies, cancellation and progress on a separate fid.

[Header](../include/styxrpc/vfs_client.h) · [Implementation](../src/vfs_client.c#L92)
