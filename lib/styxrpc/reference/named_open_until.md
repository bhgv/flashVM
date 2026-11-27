# styxrpc_named_open_until

[Русский](named_open_until.ru.md) · [Named channels](named_channel.md) ·
[Declaration](../include/styxrpc/named_channel.h) · [Implementation](../src/named_channel_client.c)

```c
#include <styxrpc/named_channel.h>

StyxRpcIoOutcome styxrpc_named_open_until(struct StyxClient *session,
    const char *path, StyxRpcVfsFile **out, StyxRpcNamedStop stop, void *state);
```

Link the hosted C target `StyxRpc::launch`. Pass an attached `session`
whose peer exports `/.channels`. The canonical absolute VFS `path` must
name a channel, fit in fewer than `STYXRPC_VFS_PATH_MAX` bytes and avoid
reserved service components. The function opens an independent participant
in an existing channel or creates a channel at a free name.

On `STYXRPC_IO_OK`, `out` receives an owned read/write fid. Release it
with `styxrpc_vfs_close`. Serialize use of the fid and close. Separate
participants have independent fids. On every failure `out` retains its
previous value.

| Outcome | Meaning |
| --- | --- |
| `STYXRPC_IO_NOT_OPEN` | Bad input or the peer could not open the service. |
| `STYXRPC_IO_FAILED` | The service refused the enrollment, a fragment was incomplete, or local preparation failed. |
| `STYXRPC_IO_CANCELLED` | The caller stopped during setup or the enrollment write. |
| `STYXRPC_IO_UNCERTAIN` | The write lost confirmation without a caller stop. |

The optional `stop(state)` runs on the calling thread and returns nonzero
to stop. It should return promptly and avoid reentering the session.
Keep `state` alive through return. NULL uses ordinary blocking waits.
On a multiplexed session, the client flushes a pending walk, open or
write by tag. In a sequential session it can check stop between exchanges.
If the entire enrollment is confirmed before flush completes, the function
can return success. On cancellation, cleanup can continue after return.
An accepted reservation can remain visible until its fid is retired.
The provider releases the name when its final participant closes.

```c
StyxRpcVfsFile *port = NULL;
StyxRpcIoOutcome joined = styxrpc_named_open_until(session, "/jobs", &port,
    stop_requested, task);
if (joined == STYXRPC_IO_OK)
{
    styxrpc_vfs_close(port, 0);
}
```

Use a paired DisVM attachment with a writable parent for `/jobs`.
Here `stop_requested(task)` reads the task's stop flag. Between opening
and closing, use [send](named_send_until.md) or [receive](named_recv_until.md)
with a matching peer. The [paired test](../tests/flashvm/mixed_pair.py)
uses `--cancel-channel-setup` to cancel an enrollment accepted by DisVM
and then open the same name again.
