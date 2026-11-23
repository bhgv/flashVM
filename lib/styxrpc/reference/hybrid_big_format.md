# styxrpc_hybrid_big_format

[Русский](hybrid_big_format.ru.md) · [Declaration](../include/styxrpc/hybrid.h#L81) · [Implementation](../src/hybrid.c) · [Table guide](hybrid_tables.md)

```c
#include <styxrpc/hybrid.h>
size_t styxrpc_hybrid_big_format(int64_t value, char *bytes, size_t capacity);
```

Render `value` as canonical signed decimal into caller-owned `bytes`. `capacity` is measured in bytes and must be at least `STYXRPC_HYBRID_BIG_TEXT` (21). The result is NUL-terminated. The return value is the number of characters excluding NUL, or zero for invalid storage. The function allocates no memory.

[Executable C fixture](../tests/hybrid.c)
