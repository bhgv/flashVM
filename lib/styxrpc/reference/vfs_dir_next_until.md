# styxrpc_vfs_dir_next_until

[Русский](vfs_dir_next_until.ru.md) · [Original API](vfs_directory.md) · [Shared contract](../VFS_CONTROL.md)

## Syntax

```c
#include <styxrpc/vfs_client.h>

StyxRpcIoOutcome styxrpc_vfs_dir_next_until(StyxRpcVfsDir *dir, StyxRpcVfsVisitor visit, void *state, int *found,
    StyxRpcVfsStop stop, void *stop_state);
```

Hosted C99, target `StyxRpc::launch`.

Advance to one visible record or EOF. Buffered records are decoded as complete entries. A cancelled step retires that fid, after which rewind refuses. Close the allocation, then open a fresh cursor to continue.

## Parameters

| Parameter | Meaning |
| --- | --- |
| `dir` | Borrowed cursor, serialized by the caller. Its allocation stays owned by the caller on every result. |
| `visit` | Optional visitor invoked once for a visible entry. Receives borrowed name and metadata. Return 1 to accept. |
| `state` | Opaque argument borrowed by visit during the call. |
| `found` | Writable flag. Set to zero at EOF or cancellation and one for an emitted entry. |
| `stop` | Optional short native callback on the calling thread. Return nonzero to stop. NULL keeps ordinary synchronous behavior. |
| `stop_state` | Opaque stop argument borrowed until return. |

## Result

Statuses below use the `STYXRPC_IO_` prefix. `OK` confirms the result. `CANCELLED` reports a stop requested by the callback. On a multiplexed client, a sent request is flushed when cancellation wins. Serial transports sample the hook before an exchange. An original acknowledgement determines the result when it wins.

An invalid handle gives `NOT_OPEN`. An invalid record or a refusing visitor gives `FAILED`. The visitor is not called for an unacknowledged entry.

## Example

Use an attached session or previously opened descriptor and a stop callback that reads your caller’s stop flag.

```c
int found;
StyxRpcIoOutcome status = styxrpc_vfs_dir_next_until(dir, NULL, NULL, &found, stop, state);
if (status == STYXRPC_IO_CANCELLED)
{
    styxrpc_vfs_dir_close(dir, 1);
}
```

The [wire test](../tests/vfs_control_wire.c) exercises this entry point. Run `ctest --test-dir build-styxrpc -R styxrpc-vfs-control-wire --output-on-failure` to check replies, cancellation and progress on a separate fid.

[Header](../include/styxrpc/vfs_client.h) · [Implementation](../src/vfs_directory.c#L109)
