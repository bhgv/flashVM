# styxrpc_graph_arguments

[Русский](graph_arguments.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/graph.h>
StyxRpcStatus styxrpc_graph_arguments(const StyxRpcSchema *schema, const void *request, size_t request_size,
    FVM_SRecord *arguments);
```

Return a borrowed view of the exact complete positional request after validating the graph. Preserve the original request storage through reply generation.

C99. `StyxRpc::rpc`.

[Example](../examples/graph_device.c) · [Source](../src/graph.c)
