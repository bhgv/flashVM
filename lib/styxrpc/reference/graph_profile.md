# styxrpc_graph_profile

[Русский](graph_profile.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/graph.h>
StyxRpcStatus styxrpc_graph_profile(const char *name, uint32_t signature, const StyxRpcSchema *schema,
    void *bytes, size_t capacity, size_t *used);
```

Produce a single .interface offer from the exact compiled signature. Publish the positional graph marker as .ref-fields in the module directory. This value-only path carries no channel grant.

C99. `StyxRpc::rpc`.

[Example](../examples/graph_device.c) · [Source](../src/graph.c)
