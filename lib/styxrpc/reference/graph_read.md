# styxrpc_graph_read

[Русский](graph_read.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/graph.h>
StyxRpcStatus styxrpc_graph_read(const StyxRpcSchema *schema, const void *request, size_t request_size,
    const void *reply, size_t reply_size, StyxRpcGraphReply *decoded);
```

Validate both complete frames against the schema and return borrowed result/update views. REMOTE_ERROR indicates a decoded provider error. The application applies the checked changes to its own objects.

C99. `StyxRpc::rpc`.

[Example](../examples/graph_device.c) · [Source](../src/graph.c)
