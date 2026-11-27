# styxrpc_managed_invoke_at_until

[Русский](managed_invoke_at_until.ru.md) · [Declaration](../include/styxrpc/managed_client.h#L44) ·
[Implementation](../src/managed_client.c#L11)

```c
#include <styxrpc/managed_client.h>

StyxRpcOutcome styxrpc_managed_invoke_at_until(struct StyxFid *directory,
    const StyxRpcManagedTarget *target, const void *arguments, size_t size,
    void *reply, size_t capacity, StyxRpcManagedReply *result,
    StyxRpcCallStop stop, void *state);
```

Use the hosted `StyxRpc::launch` target. Borrow the exact unopened
module `directory` or pass NULL to select the ordinary path, as in
[`styxrpc_managed_invoke_at`](managed_invoke_at.md).

Borrow the checked target, encoded argument records and `size` bytes.
Provide a writable reply buffer of `capacity` bytes. The target supplies
the schema, optional scoped PID and the reply polling budget. On
success the decoded result, including any reference updates, borrows
the reply buffer. Only a complete checked reply supplies updates.

The optional hook `stop(state)` runs synchronously on the calling
thread. It should sample a stop flag and return nonzero to stop.
The function checks before opening the call, before writing and
between request fragments and reply polls. A NULL hook gives ordinary
blocking behavior. Keep its state alive through return and leave the
owned per-call wire fid to the SDK. In-flight I/O and clunk obey the
mounted session's deadlines. An already received complete reply keeps
its ordinary outcome.

| Outcome | Meaning |
| --- | --- |
| `STYXRPC_CALL_CANCELLED` | The hook stopped the call before a request write. |
| `STYXRPC_CALL_CANCELLED_UNCERTAIN` | The hook stopped the call after a write. Peer effects may have occurred. |

The SDK closes the per-call fid on cancellation and does not replay
a request. It preserves the typed output on cancellation. The ordinary
success and failure outcomes also apply. The module directory remains
borrowed. Use distinct buffers and outputs for concurrent calls.

```c
StyxRpcOutcome outcome = styxrpc_managed_invoke_at_until(
    directory, &target, arguments, size, reply, sizeof(reply), &result,
    stop_requested, task);
```

Here `stop_requested(task)` reads the caller's stop flag. The
[paired test](../tests/flashvm/mixed_pair.py) uses `--cancel-dis-call`
to check cancellation and later calls on the same Styx connection.
