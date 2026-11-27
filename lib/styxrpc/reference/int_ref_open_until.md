# styxrpc_int_ref_open_until

[Русский](int_ref_open_until.ru.md) · [Declaration](../include/styxrpc/variable_ref.h#L42) · [Implementation](../src/variable_ref_client.c#L14)

Resolve an issued temporary process Int32 reference on its original session.

## Syntax

```c
#include <styxrpc/variable_ref.h>

StyxRpcIoOutcome styxrpc_int_ref_open_until(struct StyxClient *issuer,
    const StyxRpcIntRef *ref, StyxRpcVfsFile **out, StyxRpcVfsStop stop, void *state);
```

## Availability

Hosted C SDK, linking `StyxRpc::launch`. Requires an attached Styx client and a provider that exposes Int32 files under `.var`. The examples are verified on Linux64 with DisVM.

## Parameters

| Name | Description |
| --- | --- |
| `issuer` | Borrowed attached Styx client for the original issuer's program tree. |
| `ref` | Borrowed decoded descriptor. Its PID is at least `STYXRPC_INT_REF_FIRST_PID` (0x80000000) and `writable` is 0 for read or 1 for read/write. |
| `out` | Required output for the owned fid. |
| `stop` | Optional callback, invoked on the calling thread. Return nonzero to request stop. NULL uses ordinary blocking waits. |
| `state` | Borrowed callback context through return. It can be NULL if accepted by the callback. |

## Return value

| Outcome | Condition |
| --- | --- |
| `STYXRPC_IO_OK` | Returns an owned fid. Close it after use. |
| `STYXRPC_IO_NOT_OPEN` | Invalid input, rejected walk or open, retired grant or mismatched PID in the Qid. |
| `STYXRPC_IO_FAILED` | Post-open stat failed, or the file had the wrong type or extent. |
| `STYXRPC_IO_CANCELLED` | Stop interrupted admission. Outputs are preserved and any temporary fid is retired. |

## Behavior and lifetime

The SDK opens `.prog/<pid>/.var/ref` with the descriptor's mode. It verifies the regular four-byte file and compares the high 32 bits of the opened `Qid.path` with the issuing PID. Revocation invalidates existing fids, and a new grant uses a fresh PID. All failures preserve the output pointers' values. Temporary fid cleanup can finish after return.

The callback should return promptly and avoid reentering the same session. Multiplexed Styx can flush a pending request by tag. Sequential sessions sample stop between exchanges.

## Example

```c
StyxRpcVfsFile *file = NULL;
StyxRpcIoOutcome result = styxrpc_int_ref_open_until(issuer, &received,
    &file, stop_requested, wait);
```

Here `received` is a descriptor decoded on the original issuer's connection. Close `file` if the open succeeded.

## See also

[Process Int32 references](int_variable_refs.md) · [`styxrpc_vfs_int_open_identified_until`](vfs_int_open_identified_until.md) · [`styxrpc_module_ref_open_remote_until`](module_ref_open_remote_until.md) · [`styxrpc_vfs_int_read_until`](vfs_int_read_until.md) · [`styxrpc_vfs_int_write_until`](vfs_int_write_until.md) · [`styxrpc_vfs_close`](vfs_close.md) · [Runnable FlashVM and DisVM example](../examples/disvm/INT32_REFERENCES.md)
