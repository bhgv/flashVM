# styxrpc_graph_invoke_at_until

[Русский](graph_invoke_at_until.ru.md) · [Declaration](../include/styxrpc/graph_client.h#L28) ·
[Implementation](../src/graph_client.c#L11)

```c
#include <styxrpc/graph_client.h>

StyxRpcOutcome styxrpc_graph_invoke_at_until(struct StyxFid *directory,
    const StyxRpcManagedTarget *target, const void *arguments, size_t size,
    void *reply, size_t capacity, StyxRpcGraphReply *result,
    StyxRpcCallStop stop, void *state);
```

Use the hosted `StyxRpc::launch` target. Borrow the exact unopened
module `directory` or pass NULL to select the ordinary path, as in
[`styxrpc_graph_invoke_at`](graph_invoke_at.md).

Borrow the checked target and `size` bytes of the encoded argument
graph. Supply a reply buffer of `capacity` bytes. The target supplies
the schema, optional scoped PID and the reply polling budget. On
success the graph reply borrows bytes from that buffer. A cancelled
call supplies neither a result graph nor positional updates.

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
StyxRpcOutcome outcome = styxrpc_graph_invoke_at_until(
    directory, &target, arguments, size, reply, sizeof(reply), &result,
    stop_requested, task);
```

Here `stop_requested(task)` reads the caller's stop flag. The
[paired test](../tests/flashvm/mixed_pair.py) uses `--cancel-dis-call`
to check cancellation and later calls on the same Styx connection.
