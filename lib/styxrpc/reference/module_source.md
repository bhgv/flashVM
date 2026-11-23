# styxrpc_module_source

[Русский](module_source.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/interface.h>
StyxRpcStatus styxrpc_module_source(const char *module, const StyxRpcDeclaration *functions, size_t count,
    const StyxRpcAdt *adts, size_t adt_count, char *bytes, size_t capacity, size_t *used);
```

Measure or emit a NUL-terminated Limbo module declaration. The schema supplies function arguments/results and the ADT descriptors supply field names. Can render arrays, tuples, typed channels and named ref ADTs. Use a disjoint buffer with space for used plus its NUL. Ask Limbo to compile it for signature verification. One ADT name is allowed per structural token.

C99. `StyxRpc::rpc`.

[Example](../examples/disvm/interface.c) · [Source](../src/interface.c)
