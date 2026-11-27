# styxrpc_vfs_open_until

[Русский](vfs_open_until.ru.md) · [Original API](vfs_open.md) · [Shared contract](../VFS_CONTROL.md)

## Syntax

```c
#include <styxrpc/vfs_client.h>

StyxRpcIoOutcome styxrpc_vfs_open_until(struct StyxClient *session, const char *path, StyxRpcFileMode mode,
    StyxRpcVfsFile **file,
    StyxRpcVfsStop stop, void *state);
```

Hosted C99, target `StyxRpc::launch`.

Walk, check metadata and open a regular file in the selected mode. When cancellation wins, the temporary fid is retired and the output pointer stays unchanged.

## Parameters

| Parameter | Meaning |
| --- | --- |
| `session` | Borrowed mounted client, kept alive until return. |
| `path` | Borrowed visible relative UTF-8 path up to 1024 bytes, with no leading slash, dot or dotdot components. |
| `mode` | STYXRPC_FILE_READ, STYXRPC_FILE_WRITE or STYXRPC_FILE_RDWR. |
| `file` | Writable output for one owned file on success. Close with styxrpc_vfs_close. |
| `stop` | Optional short native callback on the calling thread. Return nonzero to stop. NULL keeps ordinary synchronous behavior. |
| `state` | Opaque stop argument borrowed until return. |

## Result

Statuses below use the `STYXRPC_IO_` prefix. `OK` confirms the result. `CANCELLED` reports a stop requested by the callback. On a multiplexed client, a sent request is flushed when cancellation wins. Serial transports sample the hook before an exchange. An original acknowledgement determines the result when it wins.

A hidden path, wrong kind or refused open returns `NOT_OPEN`. The new fid is published only after `Ropen`.

## Example

Use an attached session or previously opened descriptor and a stop callback that reads your caller’s stop flag.

```c
StyxRpcVfsFile *file = NULL;
StyxRpcIoOutcome status = styxrpc_vfs_open_until(session, "item",
    STYXRPC_FILE_READ, &file, stop, state);
styxrpc_vfs_close(file, 0);
```

The [wire test](../tests/vfs_control_wire.c) exercises this entry point. Run `ctest --test-dir build-styxrpc -R styxrpc-vfs-control-wire --output-on-failure` to check replies, cancellation and progress on a separate fid.

[Header](../include/styxrpc/vfs_client.h) · [Implementation](../src/vfs_client.c#L210)
