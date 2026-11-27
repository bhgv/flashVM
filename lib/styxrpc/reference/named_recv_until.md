# styxrpc_named_recv_until

[Русский](named_recv_until.ru.md) · [Named channels](named_channel.md) · [Header](../include/styxrpc/named_channel.h)

Receive a named-channel frame with cancellation.

Include `styxrpc/named_channel.h`. Pass an open channel fid `file`
from `styxrpc_named_open`. On success, `bytes` owns the received buffer for `free` and `size` is its byte count. Both outputs stay unchanged on failure.

The optional `stop(state)` hook runs synchronously before submission
and between reply polls. A nonzero return cancels the local wait. The
hook must not use this fid. A null hook gives the normal blocking wait.

On `STYXRPC_IO_CANCELLED`, close the fid with `styxrpc_vfs_close`.
A submitted operation may already have matched, so avoid a blind retry.
Cancellation does not interrupt an active Styx transaction. Other fids
can progress between polls. Serialize operations on the same fid.
