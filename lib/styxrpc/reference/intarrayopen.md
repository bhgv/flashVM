# styxintarrayopen

[Русский](intarrayopen.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/indexed_variable.h>
uint64_t styxintarrayopen(StyxIntArray *array, const void *client, int selector);
```

Open a selector (selector nonzero) or bind a value to the currently selected generation (zero). The borrowed client identity must be stable. Return a nonzero generation or zero on refusal.

`StyxRpc::rpc`.

[Example](../examples/disvm/reverse.c) · [Source](../protocol/indexed_variable.c)
