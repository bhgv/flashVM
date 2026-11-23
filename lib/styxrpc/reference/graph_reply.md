# styxrpc_graph_reply

[Русский](graph_reply.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/graph.h>
StyxRpcStatus styxrpc_graph_reply(const StyxRpcSchema *schema, const void *request, size_t request_size,
    const void *result, size_t result_size, const void *updates, size_t update_size,
    void *bytes, size_t capacity, size_t *used);
```

Seal two already encoded records, the return and sparse edits. Validate both against the original request and its reference identities before writing. Keep the output disjoint.

C99. `StyxRpc::rpc`.

[Example](../examples/graph_device.c) · [Source](../src/graph.c)
