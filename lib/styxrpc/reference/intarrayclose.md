# styxintarrayclose

[Русский](intarrayclose.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/indexed_variable.h>
void styxintarrayclose(StyxIntArray *array, uint64_t generation);
```

Release this exact selector generation. All corresponding value fids become stale. Pass only the generation owned by a selector.

`StyxRpc::rpc`.

[Example](../examples/disvm/reverse.c) · [Source](../protocol/indexed_variable.c)
