# styxrpc_scalar_invoke

[Русский](scalar_invoke.ru.md) · [Header](../include/styxrpc/scalar_client.h) · [Implementation](../src/scalar_client.c)

Include `styxrpc/scalar_client.h` and link `StyxRpc::launch` from the hosted SDK.

```c
StyxRpcOutcome styxrpc_scalar_invoke(struct StyxClient *session, const char *module,
    const char *name, uint32_t signature, int64_t argument, unsigned wait_ms, int64_t *result);
```

Supply an already mounted Styx session, a module path relative to the mount and
the exact export component. `signature` is `STYXRPC_INT_SIGNATURE` or
`STYXRPC_BIG_SIGNATURE`. `argument` must fit that signed type. The call borrows
all storage. It holds one module fid to verify `.interface` and walk the function
file, then owns that function fid through the reply. The verifier is the same one
used by DisVM's loader. This entry point supports exports that require no caller
PID. Read/execute access on the module and read/write/execute access on the
function are required.

| Outcome | Meaning |
| --- | --- |
| `STYXRPC_CALL_OK` | The reply was checked and `result` was written. |
| `STYXRPC_CALL_REMOTE_ERROR` | The peer returned a framed application error. |
| `STYXRPC_CALL_NOT_SENT` | No function request write was attempted. |
| `STYXRPC_CALL_UNCERTAIN` | A write was attempted and no checked outcome was obtained. It may have executed. |

The client polls for a pending reply for up to `wait_ms` (1–60000 ms).
Styx operations and clunk have their own session timeouts. The call is
synchronous, continues only a confirmed short write, and never replays a call.
Keep the session alive until all calls return. Do not call on an event or render
thread that must stay responsive. See the [standalone C client](../examples/client.c)
and [module publication](scalar_module_init.md).
