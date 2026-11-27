# styxrpc_argument_scope_invoke_at_until

[Русский](argument_scope_invoke_at_until.ru.md) · [Declaration](../include/styxrpc/channel_arguments.h#L79) ·
[Implementation](../src/channel_client.c#L149)

```c
#include <styxrpc/channel_arguments.h>

StyxRpcOutcome styxrpc_argument_scope_invoke_at_until(struct StyxFid *directory,
    StyxRpcArgumentScope *scope, const StyxRpcManagedTarget *target,
    const void *arguments, size_t size, void *reply, size_t capacity,
    StyxRpcLeasedReply *result,
    StyxRpcCallStop stop, void *state);
```

Use the hosted `StyxRpc::launch` target. Borrow the exact unopened
module `directory` or pass NULL to select the ordinary path, as in
[`styxrpc_argument_scope_invoke_at`](argument_scope_invoke_at.md).

Pass the checked target, including its caller PID and wait budget.
Borrow `size` bytes of encoded argument records and a `capacity`-byte
reply buffer. If supplied, the staged argument scope is consumed on
every outcome. A NULL scope uses the ordinary channel contract and
lets the caller manage any argument grants. After cancellation the
temporary scope is revoked. Confirmed queue transfers remain visible.
On success, `result` borrows the reply buffer and may own a
[channel lease](channel_lease_release.md) to release after use.

The optional hook `stop(state)` runs synchronously on the calling
thread. It should sample a stop flag and return nonzero to stop.
The function checks before opening the call and when sending or awaiting
data. On a multiplexed session, a transmitted request gets one `Tflush`
by tag. Metadata, flush acknowledgement and clunk use the session's
deadlines. A complete reply received before flush acknowledgement keeps
its ordinary outcome. A NULL hook gives ordinary blocking behavior.
Keep the state alive through return. The SDK owns the operation fid.

| Outcome | Meaning |
| --- | --- |
| `STYXRPC_CALL_CANCELLED` | The hook stopped the call before a request write. |
| `STYXRPC_CALL_CANCELLED_UNCERTAIN` | The hook stopped the call during or after a write. Peer effects may have occurred. |

The SDK closes the per-call fid on cancellation and does not replay
a request. It preserves the typed output on cancellation. The ordinary
success and failure outcomes also apply. The module directory remains
borrowed. Use distinct buffers and outputs for concurrent calls.

```c
StyxRpcOutcome outcome = styxrpc_argument_scope_invoke_at_until(
    directory, scope, &target, arguments, size, reply, sizeof(reply), &result,
    stop_requested, task);
```

Here `stop_requested(task)` reads the caller's stop flag. The
[paired test](../tests/flashvm/mixed_pair.py) uses `--cancel-dis-call`
to check cancellation and later calls on the same Styx connection.
