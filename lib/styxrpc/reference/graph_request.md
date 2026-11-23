# styxrpc_graph_request

[Русский](graph_request.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/graph.h>
StyxRpcStatus styxrpc_graph_request(const StyxRpcSchema *schema, const void *payload,
    size_t size, void *bytes, size_t capacity, size_t *used);
```

Validate the encoded argument children and write a complete tag-112 S-record. The caller supplies all storage and keeps the request until the reply is checked.

C99. `StyxRpc::rpc`.

[Example](../examples/graph_device.c) · [Source](../src/graph.c)
