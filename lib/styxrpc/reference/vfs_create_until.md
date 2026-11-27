# styxrpc_vfs_create_until

[Русский](vfs_create_until.ru.md) · [Original API](vfs_create.md) · [Shared contract](../VFS_CONTROL.md)

## Syntax

```c
#include <styxrpc/vfs_client.h>

StyxRpcIoOutcome styxrpc_vfs_create_until(struct StyxClient *session, const char *path, StyxRpcVfsFile **file,
    StyxRpcVfsStop stop, void *state);
```

Hosted C99, target `StyxRpc::launch`.

Validate the existing parent, then create one regular file with requested mode 0666 and open it for writing. Serialize later uses of a returned file.

## Parameters

| Parameter | Meaning |
| --- | --- |
| `session` | Borrowed mounted client, kept alive until return. |
| `path` | Borrowed visible relative UTF-8 path up to 1024 bytes, with no leading slash, dot or dotdot components. |
| `file` | Writable output for one owned file on success. Close with styxrpc_vfs_close. |
| `stop` | Optional short native callback on the calling thread. Return nonzero to stop. NULL keeps ordinary synchronous behavior. |
| `state` | Opaque stop argument borrowed until return. |

## Result

Statuses below use the `STYXRPC_IO_` prefix. `OK` confirms the result. `CANCELLED` reports a stop requested by the callback. On a multiplexed client, a sent request is flushed when cancellation wins. Serial transports sample the hook before an exchange. An original acknowledgement determines the result when it wins.

A refused parent gives `NOT_OPEN`. A definitive mutation error gives `FAILED`. A sent create without acknowledgement gives `UNCERTAIN` and may leave a file. The SDK does not replay it.

## Example

Use an attached session or previously opened descriptor and a stop callback that reads your caller’s stop flag.

```c
StyxRpcVfsFile *file = NULL;
StyxRpcIoOutcome status = styxrpc_vfs_create_until(session, "new", &file, stop, state);
styxrpc_vfs_close(file, 0);
```

The [wire test](../tests/vfs_control_wire.c) exercises this entry point. Run `ctest --test-dir build-styxrpc -R styxrpc-vfs-control-wire --output-on-failure` to check replies, cancellation and progress on a separate fid.

[Header](../include/styxrpc/vfs_client.h) · [Implementation](../src/vfs_create.c#L99)
