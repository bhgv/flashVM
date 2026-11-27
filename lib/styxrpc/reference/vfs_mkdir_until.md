# styxrpc_vfs_mkdir_until

[Русский](vfs_mkdir_until.ru.md) · [Original API](vfs_mkdir.md) · [Shared contract](../VFS_CONTROL.md)

## Syntax

```c
#include <styxrpc/vfs_client.h>

StyxRpcIoOutcome styxrpc_vfs_mkdir_until(struct StyxClient *session, const char *path,
    StyxRpcVfsStop stop, void *state);
```

Hosted C99, target `StyxRpc::launch`.

Validate the existing parent and create one directory with requested mode 0777. The SDK owns and retires the temporary directory fid.

## Parameters

| Parameter | Meaning |
| --- | --- |
| `session` | Borrowed mounted client, kept alive until return. |
| `path` | Borrowed visible relative UTF-8 path up to 1024 bytes, with no leading slash, dot or dotdot components. |
| `stop` | Optional short native callback on the calling thread. Return nonzero to stop. NULL keeps ordinary synchronous behavior. |
| `state` | Opaque stop argument borrowed until return. |

## Result

Statuses below use the `STYXRPC_IO_` prefix. `OK` confirms the result. `CANCELLED` reports a stop requested by the callback. On a multiplexed client, a sent request is flushed when cancellation wins. Serial transports sample the hook before an exchange. An original acknowledgement determines the result when it wins.

A refused parent gives `NOT_OPEN`. A definitive mutation error gives `FAILED`. A sent create without acknowledgement gives `UNCERTAIN`; check the tree before taking further action.

## Example

Use an attached session or previously opened descriptor and a stop callback that reads your caller’s stop flag.

```c
StyxRpcIoOutcome status = styxrpc_vfs_mkdir_until(session, "folder", stop, state);
```

The [wire test](../tests/vfs_control_wire.c) exercises this entry point. Run `ctest --test-dir build-styxrpc -R styxrpc-vfs-control-wire --output-on-failure` to check replies, cancellation and progress on a separate fid.

[Header](../include/styxrpc/vfs_client.h) · [Implementation](../src/vfs_create.c#L114)
