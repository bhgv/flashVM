# styxintarrayread

[Русский](intarrayread.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/indexed_variable.h>
int styxintarrayread(StyxIntArray *array, uint64_t generation, uint64_t offset, void *bytes, size_t *size);
```

Read a signed Int32 from its live selector generation. Supply output capacity in size; on success size contains byte count. EOF starts at offset four.

`StyxRpc::rpc`.

[Example](../examples/disvm/reverse.c) · [Source](../protocol/indexed_variable.c)
