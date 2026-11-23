# styxrpc_hybrid_open

[Русский](hybrid_open.ru.md) · [Declaration](../include/styxrpc/hybrid.h#L45) · [Implementation](../src/hybrid.c) · [Table guide](hybrid_tables.md)

```c
#include <styxrpc/hybrid.h>
StyxRpcHybrid *styxrpc_hybrid_open(const void *snapshot, size_t size);
```

Open an independently owned table from one complete Dis snapshot in `snapshot`. `size` is its byte count and may be at most 64 KiB. Pass `NULL, 0` to create an empty table. The function copies keys and values before it returns and rejects malformed input or duplicate keys. It returns the owned table or `NULL` on failure. Release it with [close](hybrid_close.md).

[Executable C fixture](../tests/hybrid.c)
