# styxrpc_vfs_int_write_until

[Русский](vfs_int_write_until.ru.md) · [Declaration](../include/styxrpc/vfs_client.h#L255) · [Implementation](../src/vfs_int.c#L109)

Replace a signed Int32 in an already opened variable.

## Syntax

```c
#include <styxrpc/vfs_client.h>

StyxRpcIoOutcome styxrpc_vfs_int_write_until(StyxRpcVfsFile *file,
    int32_t value, StyxRpcVfsStop stop, void *state);
```

## Availability

Hosted C SDK, linking `StyxRpc::launch`. Requires an attached Styx client and a provider that exposes Int32 files under `.var`. The examples are verified on Linux64 with DisVM.

## Parameters

| Name | Description |
| --- | --- |
| `file` | Borrowed open Int32 fid with write access. Serialize operations and close on it. |
| `value` | Any signed value from −2147483648 through 2147483647. |
| `stop` | Optional callback, invoked on the calling thread. Return nonzero to request stop. NULL uses ordinary blocking waits. |
| `state` | Borrowed callback context through return. It can be NULL if accepted by the callback. |

## Return value

| Outcome | Condition |
| --- | --- |
| `STYXRPC_IO_OK` | All four bytes were acknowledged. |
| `STYXRPC_IO_NOT_OPEN` | NULL file. |
| `STYXRPC_IO_FAILED` | Confirmed rejection or short acknowledgement. |
| `STYXRPC_IO_CANCELLED` | Stop prevented submission of a potentially effective write. |
| `STYXRPC_IO_UNCERTAIN` | A submitted write may have taken effect without an acknowledgement. |

## Behavior and lifetime

The SDK sends one `Twrite` for four signed big-endian bytes at offset zero. Success requires acknowledgement of all four bytes. An original full acknowledgement delivered before `Rflush` can still complete the write. The caller still owns `file`. After an in-flight cancellation or uncertain outcome, retire the fid and reopen by the original descriptor if needed. Avoid automatically replaying an uncertain assignment.

The callback should return promptly and avoid reentering the same session. Multiplexed Styx can flush a pending request by tag. Sequential sessions sample stop between exchanges.

## Example

```c
StyxRpcIoOutcome result = styxrpc_vfs_int_write_until(file, -73,
    stop_requested, wait);
```

After an uncertain write, a new exact open and a read can show the current value. Another writer may have updated it in the meantime.

## See also

[`styxrpc_vfs_int_open_identified_until`](vfs_int_open_identified_until.md) · [`styxrpc_module_ref_open_remote_until`](module_ref_open_remote_until.md) · [`styxrpc_vfs_int_read_until`](vfs_int_read_until.md) · [`styxrpc_vfs_close`](vfs_close.md) · [Runnable FlashVM and DisVM example](../examples/disvm/INT32_REFERENCES.md)
