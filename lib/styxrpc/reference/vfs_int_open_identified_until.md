# styxrpc_vfs_int_open_identified_until

[Русский](vfs_int_open_identified_until.ru.md) · [Declaration](../include/styxrpc/vfs_client.h#L240) · [Implementation](../src/vfs_int.c#L10)

Open a mounted Int32 and optionally retrieve the identity of its opened file.

## Syntax

```c
#include <styxrpc/vfs_client.h>

StyxRpcIoOutcome styxrpc_vfs_int_open_identified_until(struct StyxClient *session, const char *path,
    int writable, StyxRpcVfsFile **out, uint64_t *identity, StyxRpcVfsStop stop, void *state);
```

## Availability

Hosted C SDK, linking `StyxRpc::launch`. Requires an attached Styx client and a provider that exposes Int32 files under `.var`. The examples are verified on Linux64 with DisVM.

## Parameters

| Name | Description |
| --- | --- |
| `session` | Borrowed attached Styx client for the selected tree. |
| `path` | NUL-terminated relative path, 1 through 1024 bytes. The final component is directly under `.var`. Empty components, `.` and `..` are invalid. |
| `writable` | 0 opens for read, 1 for read/write. |
| `out` | Required output for the owned fid. |
| `identity` | Optional output for the full unsigned 64-bit `Qid.path` from the opened fid. |
| `stop` | Optional callback, invoked on the calling thread. Return nonzero to request stop. NULL uses ordinary blocking waits. |
| `state` | Borrowed callback context through return. It can be NULL if accepted by the callback. |

## Return value

| Outcome | Condition |
| --- | --- |
| `STYXRPC_IO_OK` | Returns an owned fid. Close it after use. |
| `STYXRPC_IO_NOT_OPEN` | Invalid input, failed walk, pre-open type check, or rejected open. |
| `STYXRPC_IO_FAILED` | Post-open stat failed, or the file had the wrong type or extent. |
| `STYXRPC_IO_CANCELLED` | Stop interrupted admission. Outputs are preserved and any temporary fid is retired. |

## Behavior and lifetime

After opening, the SDK calls stat on the same fid and verifies a regular four-byte file. It publishes the fid and optional identity together. Capture the identity at this point when exporting a descriptor for this publication. All failures preserve the output pointers' values. Temporary fid cleanup can finish after return.

The callback should return promptly and avoid reentering the same session. Multiplexed Styx can flush a pending request by tag. Sequential sessions sample stop between exchanges.

## Example

```c
StyxRpcVfsFile *file = NULL;
uint64_t qid;
StyxRpcIoOutcome result = styxrpc_vfs_int_open_identified_until(session,
    "mounted/counter.dis/.var/value", 0, &file, &qid, stop_requested, wait);
```

Use `qid` and `file` only after success. Keep the session alive through the final close.

## See also

[`styxrpc_module_ref_open_remote_until`](module_ref_open_remote_until.md) · [`styxrpc_vfs_int_read_until`](vfs_int_read_until.md) · [`styxrpc_vfs_int_write_until`](vfs_int_write_until.md) · [`styxrpc_vfs_close`](vfs_close.md) · [Runnable FlashVM and DisVM example](../examples/disvm/INT32_REFERENCES.md)
