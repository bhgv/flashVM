# styxrpc_argument_scope_invoke

[Русский](argument_scope_invoke.ru.md) · [Declaration](../include/styxrpc/channel_arguments.h#L57) · [Implementation](../src/channel_client.c) · [Scope guide](channel_argument_scopes.md)

Link `StyxRpc::launch`.

```c
#include <styxrpc/channel_arguments.h>
StyxRpcOutcome styxrpc_argument_scope_invoke(StyxRpcArgumentScope *scope, const StyxRpcManagedTarget *target,
    const void *arguments, size_t size, void *reply, size_t capacity, StyxRpcLeasedReply *result);
```

Consume the owned staging `scope` on every outcome. Pass the exact `target`, argument children with `size` in bytes, caller-owned `reply` and `capacity`, and `result` as in [channel invoke](channel_invoke.md). The target PID and validated argument keys and shapes must match the scope. After checking the compiled offer on the mounted module, the SDK activates these keys just before submission. Serve the same connection's reverse bus concurrently with the forward call. Result records borrow the reply buffer and successful result channels have their independent lease. The scope is retired before this function returns. An uncertain outcome may include prior completed queue operations.

[C regression](../tests/channel_arguments.c)
