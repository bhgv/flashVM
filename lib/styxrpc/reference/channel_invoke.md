# styxrpc_channel_invoke

[Русский](channel_invoke.ru.md) · [Declaration](../include/styxrpc/channel_client.h#L26) · [Implementation](../src/channel_client.c) · [Lease guide](channel_leases.md)

Link `StyxRpc::launch`.

```c
#include <styxrpc/channel_client.h>
StyxRpcOutcome styxrpc_channel_invoke(const StyxRpcManagedTarget *target, const void *arguments,
    size_t size, void *reply, size_t capacity, StyxRpcLeasedReply *result);
```

Pass a mounted `target` with the exact compiled module path, name and signature, complete schema, nonzero published caller PID and a reply polling budget of 1–60000 milliseconds. `arguments` contains encoded children, with `NULL, 0` for an empty list. `reply` is writable nonoverlapping storage of 1–65536 bytes. The leased-call profile accepts copied values and channel shapes. ADT refs and hybrid tables use their graph API. The caller owns and serializes the mounted session and keeps its reverse publication available.

`STYXRPC_CALL_OK` publishes `result` with a record borrowing `reply`, a copy of the distinct routes and one owned lease reference when a non-nil channel was returned. Release it with [lease release](channel_lease_release.md). The API verifies the scoped offer and both capability markers on the same module fid before writing. `STYXRPC_CALL_NOT_SENT` is a known failure before submission. `STYXRPC_CALL_REMOTE_ERROR` confirms a remote error. `STYXRPC_CALL_UNCERTAIN` leaves the execution unresolved. The function submits once. For locally owned argument queues, use [scoped argument invoke](argument_scope_invoke.md).

[Executable fixture](../examples/disvm/routed_client.c)
