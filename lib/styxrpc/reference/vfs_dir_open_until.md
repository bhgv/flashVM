# styxrpc_vfs_dir_open_until

[Русский](vfs_dir_open_until.ru.md) · [Original API](vfs_directory.md) · [Shared contract](../VFS_CONTROL.md)

## Syntax

```c
#include <styxrpc/vfs_client.h>

StyxRpcIoOutcome styxrpc_vfs_dir_open_until(struct StyxClient *session, const char *path, StyxRpcVfsDir **out,
    StyxRpcVfsStop stop, void *state);
```

Hosted C99, target `StyxRpc::launch`.

Walk, check and open a directory for reading. The SDK retires temporary fids if cancellation wins and transfers an independent cursor on success.

## Parameters

| Parameter | Meaning |
| --- | --- |
| `session` | Borrowed mounted client, kept alive until return. |
| `path` | Borrowed visible relative UTF-8 path up to 1024 bytes. An empty string denotes the attach root. |
| `out` | Writable output for one owned cursor on success. Close with styxrpc_vfs_dir_close. |
| `stop` | Optional short native callback on the calling thread. Return nonzero to stop. NULL keeps ordinary synchronous behavior. |
| `state` | Opaque stop argument borrowed until return. |

## Result

Statuses below use the `STYXRPC_IO_` prefix. `OK` confirms the result. `CANCELLED` reports a stop requested by the callback. On a multiplexed client, a sent request is flushed when cancellation wins. Serial transports sample the hook before an exchange. An original acknowledgement determines the result when it wins.

An unavailable or invalid directory gives `NOT_OPEN`. Allocation failure gives `FAILED`. Other results leave the caller’s output pointer intact.

## Example

Use an attached session or previously opened descriptor and a stop callback that reads your caller’s stop flag.

```c
StyxRpcVfsDir *dir = NULL;
StyxRpcIoOutcome status = styxrpc_vfs_dir_open_until(session, "", &dir, stop, state);
styxrpc_vfs_dir_close(dir, 0);
```

The [wire test](../tests/vfs_control_wire.c) exercises this entry point. Run `ctest --test-dir build-styxrpc -R styxrpc-vfs-control-wire --output-on-failure` to check replies, cancellation and progress on a separate fid.

[Header](../include/styxrpc/vfs_client.h) · [Implementation](../src/vfs_directory.c#L20)
