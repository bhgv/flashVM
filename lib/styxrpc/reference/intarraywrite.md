# styxintarraywrite

[Русский](intarraywrite.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/indexed_variable.h>
int styxintarraywrite(StyxIntArray *array, uint64_t generation, uint64_t offset, const void *bytes, size_t size);
```

Write exactly four signed network-order bytes at offset zero on the still-live generation. Invoke the store callback once and report acceptance.

`StyxRpc::rpc`.

[Example](../examples/disvm/reverse.c) · [Source](../protocol/indexed_variable.c)
