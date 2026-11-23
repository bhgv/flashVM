# styxrpc_managed_request

[Русский](managed_request.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/managed.h>
StyxRpcStatus styxrpc_managed_request(uint32_t signature, const StyxRpcSchema *schema, const void *payload,
    size_t size, void *bytes, size_t capacity, size_t *used);
```

Check size encoded child bytes against the compiled signature and schema and seal the request into disjoint caller buffer bytes. Keep used encoded bytes to validate the reply. Invalid input leaves used zero.

C99. Serialize operations on the same fid. `StyxRpc::rpc`.

[Example](../examples/../tests/managed.c) · [Source](../src/managed.c)
