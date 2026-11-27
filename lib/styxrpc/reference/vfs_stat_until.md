# styxrpc_vfs_stat_until

[Русский](vfs_stat_until.ru.md) · [Original API](vfs_stat.md) · [Shared contract](../VFS_CONTROL.md)

## Syntax

```c
#include <styxrpc/vfs_client.h>

StyxRpcIoOutcome styxrpc_vfs_stat_until(struct StyxClient *session, const char *path, StyxRpcFileInfo *info,
    StyxRpcVfsStop stop, void *state);
```

Hosted C99, target `StyxRpc::launch`.

Walk and query an ordinary path. The temporary fid is retired after the result. A root or directory query reports metadata with the same rules as the original API.

## Parameters

| Parameter | Meaning |
| --- | --- |
| `session` | Borrowed mounted client, kept alive until return. |
| `path` | Borrowed visible relative UTF-8 path up to 1024 bytes. An empty string denotes the attach root. |
| `info` | Writable metadata output. Only success replaces its length in bytes, mode and directory flag. |
| `stop` | Optional short native callback on the calling thread. Return nonzero to stop. NULL keeps ordinary synchronous behavior. |
| `state` | Opaque stop argument borrowed until return. |

## Result

Statuses below use the `STYXRPC_IO_` prefix. `OK` confirms the result. `CANCELLED` reports a stop requested by the callback. On a multiplexed client, a sent request is flushed when cancellation wins. Serial transports sample the hook before an exchange. An original acknowledgement determines the result when it wins.

`NOT_OPEN` covers missing or hidden paths. A failed metadata reply returns `FAILED`.

## Example

Use an attached session or previously opened descriptor and a stop callback that reads your caller’s stop flag.

```c
StyxRpcFileInfo info;
StyxRpcIoOutcome status = styxrpc_vfs_stat_until(session, "item", &info, stop, state);
```

The [wire test](../tests/vfs_control_wire.c) exercises this entry point. Run `ctest --test-dir build-styxrpc -R styxrpc-vfs-control-wire --output-on-failure` to check replies, cancellation and progress on a separate fid.

[Header](../include/styxrpc/vfs_client.h) · [Implementation](../src/vfs_client.c#L118)
