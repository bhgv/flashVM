# styxrpc_named_send_until

[Русский](named_send_until.ru.md) · [Named channels](named_channel.md) · [Header](../include/styxrpc/named_channel.h)

Send a named-channel frame with cancellation.

Include `styxrpc/named_channel.h`. Pass an open channel fid `file`
from `styxrpc_named_open`. Pass nonempty borrowed `bytes` of `size` bytes. Keep the buffer valid until the call returns.

The optional `stop(state)` hook runs on the calling thread before
submission and while a multiplexed data reply is pending. Return nonzero
to request cancellation. The callback must finish promptly and must
leave the operation fid alone. A NULL hook gives the normal blocking wait.

On `STYXRPC_IO_CANCELLED`, close the fid with `styxrpc_vfs_close`.
A submitted operation may already have matched, so avoid a blind retry.
A multiplexed session flushes a transmitted request by tag. The peer
must acknowledge the flush within the session deadline. Concurrent calls
on other fids can progress while the session stays healthy. Serialize
operations on the same fid.
