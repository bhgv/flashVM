# styxrpc_argument_scope_nested

[Русский](argument_scope_nested.ru.md) · [Declaration](../include/styxrpc/channel_arguments.h) · [Implementation](../src/channel_arguments.c) · [Scope guide](channel_argument_scopes.md)

Include `<styxrpc/channel_arguments.h>` and link `StyxRpc::launch`.

```c
StyxRpcStatus styxrpc_argument_scope_nested(StyxRpcArgumentScope *scope,
    StyxRpcChannelQueue *queue, StyxRpcChannelGrant *grant);
```

Before activating `scope`, pass an application-owned `queue` and caller-owned `grant` storage. On `STYXRPC_OK`, the grant contains a fresh positive key and the queue's element shape. Repeated registration of the same queue reuses its key. This function does not add a direct argument to the expected forward frame. To include the queue directly as well, call [styxrpc_argument_scope_value](argument_scope_value.md) with the same queue.

Use the returned keys when preparing nested frames for the reverse bus. [Remap queued messages](channel_element_remap.md) from application-local keys if necessary. Activation validates nested typed frames against the complete staged grant table. There are up to eight queues in a scope, including the direct and nested queues together. Drop or retry a staging scope after `STYXRPC_INVALID`, which leaves the output unchanged. Causes include an absent or activated scope, invalid pointers and exhaustion of slots or keys.

Use the forward application owner for staging. Keep queue descriptors and slots stable until the scope retires. The reverse dispatcher operates on them after activation and retirement returns their live cursors. [Standalone regression](../tests/channel_arguments.c)
