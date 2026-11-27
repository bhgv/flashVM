# styxrpc_scalar_invoke_at_until

[Русский](scalar_invoke_at_until.ru.md) · [Declaration](../include/styxrpc/scalar_client.h#L47) ·
[Implementation](../src/scalar_client.c#L9)

```c
#include <styxrpc/scalar_client.h>

StyxRpcOutcome styxrpc_scalar_invoke_at_until(struct StyxFid *directory,
    struct StyxClient *session, const char *module, const char *name,
    uint32_t signature, int64_t argument, unsigned wait_ms, int64_t *result,
    StyxRpcCallStop stop, void *state);
```

Use the hosted `StyxRpc::launch` target. Borrow the exact unopened
module `directory` or pass NULL to select the ordinary path, as in
[`styxrpc_scalar_invoke_at`](scalar_invoke_at.md).

Pass the exact compiled signature and one signed argument. With a NULL
directory, pass the mounted session and the relative module path. The
positive `wait_ms` is the reply polling budget in milliseconds, limited
to `STYXRPC_SCALAR_WAIT_MAX_MS`. On success `result` receives the scalar.

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
StyxRpcOutcome outcome = styxrpc_scalar_invoke_at_until(
    directory, session, "lib/counter.dis", "next",
    STYXRPC_INT_SIGNATURE, 41, 1000, &result,
    stop_requested, task);
```

Here `stop_requested(task)` reads the caller's stop flag. The
[paired test](../tests/flashvm/mixed_pair.py) uses `--cancel-dis-call`
to check cancellation and later calls on the same Styx connection.
