# styxintarrayselect

[Русский](intarrayselect.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/indexed_variable.h>
int styxintarrayselect(StyxIntArray *array, uint64_t generation, uint64_t offset, const void *bytes, size_t size);
```

Write exactly four big-endian key bytes at offset zero to one live selector. Validate position or key existence before setting it. Return 1 on acceptance, 0 otherwise.

`StyxRpc::rpc`.

[Example](../examples/disvm/reverse.c) · [Source](../protocol/indexed_variable.c)
