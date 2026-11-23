# styxrpc_hybrid_stage

[Русский](hybrid_stage.ru.md) · [Declaration](../include/styxrpc/hybrid.h#L71) · [Implementation](../src/hybrid.c) · [Table guide](hybrid_tables.md)

```c
#include <styxrpc/hybrid.h>
StyxRpcHybrid *styxrpc_hybrid_stage(const StyxRpcHybrid *table, const void *patch, size_t size);
```

Copy current `table` contents and apply one complete `patch` of `size` bytes, up to 64 KiB. Only keys mentioned by the patch change. The original table stays intact. On success the returned table is independently owned and can replace the original after the other parts of a graph reply have been checked. Invalid input or allocation failure returns `NULL`. Keep the current table stable during staging and [close](hybrid_close.md) either retired or discarded storage.

[Executable C fixture](../tests/hybrid.c)
