# styxrpc_vfs_int_read_until

[Русский](vfs_int_read_until.ru.md) · [Declaration](../include/styxrpc/vfs_client.h#L248) · [Implementation](../src/vfs_int.c#L78)

Read one signed Int32 from an already opened variable.

## Syntax

```c
#include <styxrpc/vfs_client.h>

StyxRpcIoOutcome styxrpc_vfs_int_read_until(StyxRpcVfsFile *file,
    int32_t *value, StyxRpcVfsStop stop, void *state);
```

## Availability

Hosted C SDK, linking `StyxRpc::launch`. Requires an attached Styx client and a provider that exposes Int32 files under `.var`. The examples are verified on Linux64 with DisVM.

## Parameters

| Name | Description |
| --- | --- |
| `file` | Borrowed open Int32 fid with read access. Serialize operations and close on it. |
| `value` | Required output for the signed result. Its previous value is preserved on failure. |
| `stop` | Optional callback, invoked on the calling thread. Return nonzero to request stop. NULL uses ordinary blocking waits. |
| `state` | Borrowed callback context through return. It can be NULL if accepted by the callback. |

## Return value

| Outcome | Condition |
| --- | --- |
| `STYXRPC_IO_OK` | Stores the complete result in `value`. |
| `STYXRPC_IO_NOT_OPEN` | NULL file or output. |
| `STYXRPC_IO_FAILED` | Confirmed rejection or short reply. |
| `STYXRPC_IO_CANCELLED` | Stop interrupted the read before the original data reply. |
| `STYXRPC_IO_UNCERTAIN` | A usable acknowledgement could not be established. The output is unchanged. |

## Behavior and lifetime

The SDK sends one `Tread` for exactly four bytes at offset zero. A complete reply gives the signed big-endian value. A short reply is a failure. An original complete reply delivered before `Rflush` can still complete the read. The caller still owns `file`. After an in-flight cancellation or uncertain outcome, retire the fid and reopen by the original descriptor if needed.

The callback should return promptly and avoid reentering the same session. Multiplexed Styx can flush a pending request by tag. Sequential sessions sample stop between exchanges.

## Example

```c
int32_t value;
StyxRpcIoOutcome result = styxrpc_vfs_int_read_until(file, &value,
    stop_requested, wait);
```

Use `value` only when the function returns `STYXRPC_IO_OK`.

## See also

[`styxrpc_vfs_int_open_identified_until`](vfs_int_open_identified_until.md) · [`styxrpc_module_ref_open_remote_until`](module_ref_open_remote_until.md) · [`styxrpc_vfs_int_write_until`](vfs_int_write_until.md) · [`styxrpc_vfs_close`](vfs_close.md) · [Runnable FlashVM and DisVM example](../examples/disvm/INT32_REFERENCES.md)
