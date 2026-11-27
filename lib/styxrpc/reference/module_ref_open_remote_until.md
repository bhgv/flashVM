# styxrpc_module_ref_open_remote_until

[Русский](module_ref_open_remote_until.ru.md) · [Declaration](../include/styxrpc/module_ref.h#L55) · [Implementation](../src/variable_ref_client.c#L51)

Resolve a mounted module's Int32 descriptor on the original owner's ordinary VFS.

## Syntax

```c
#include <styxrpc/module_ref.h>

StyxRpcIoOutcome styxrpc_module_ref_open_remote_until(struct StyxClient *issuer,
    const StyxRpcModuleRef *ref, StyxRpcVfsFile **out, StyxRpcVfsStop stop, void *state);
```

## Availability

Hosted C SDK, linking `StyxRpc::launch`. Requires an attached Styx client and a provider that exposes Int32 files under `.var`. The examples are verified on Linux64 with DisVM.

## Parameters

| Name | Description |
| --- | --- |
| `issuer` | Borrowed attached client for the descriptor owner's ordinary VFS. Select this session using the negotiated owner role before calling. |
| `ref` | Borrowed decoded descriptor with nonzero 64-bit node identity, a canonical NUL-terminated `mounted/<image>/.var/<export>` path and mode 0 for read or 1 for read/write. |
| `out` | Required output for the owned fid. |
| `stop` | Optional callback, invoked on the calling thread. Return nonzero to request stop. NULL uses ordinary blocking waits. |
| `state` | Borrowed callback context through return. It can be NULL if accepted by the callback. |

## Return value

| Outcome | Condition |
| --- | --- |
| `STYXRPC_IO_OK` | Returns an owned fid. Close it after use. |
| `STYXRPC_IO_NOT_OPEN` | Invalid input, rejected walk or open, withdrawn export or mismatched Qid. |
| `STYXRPC_IO_FAILED` | Post-open stat failed, or the file had the wrong type or extent. |
| `STYXRPC_IO_CANCELLED` | Stop interrupted admission. Outputs are preserved and any temporary fid is retired. |

## Behavior and lifetime

The SDK opens the path, verifies a regular four-byte file and compares all 64 bits of its `Qid.path` with `ref->node`. A replacement at the same path fails the identity check. The caller validates the connection scope when decoding the descriptor. All failures preserve the output pointers' values. Temporary fid cleanup can finish after return.

The callback should return promptly and avoid reentering the same session. Multiplexed Styx can flush a pending request by tag. Sequential sessions sample stop between exchanges.

## Example

```c
StyxRpcVfsFile *file = NULL;
StyxRpcIoOutcome result = styxrpc_module_ref_open_remote_until(issuer,
    &received, &file, stop_requested, wait);
```

Here `received` is a scoped module descriptor. After cancellation, a new open can use the same descriptor to recheck the original identity.

## See also

[Module reference protocol](module_refs.md) · [`styxrpc_vfs_int_open_identified_until`](vfs_int_open_identified_until.md) · [`styxrpc_vfs_int_read_until`](vfs_int_read_until.md) · [`styxrpc_vfs_int_write_until`](vfs_int_write_until.md) · [`styxrpc_vfs_close`](vfs_close.md) · [Runnable FlashVM and DisVM example](../examples/disvm/INT32_REFERENCES.md)
