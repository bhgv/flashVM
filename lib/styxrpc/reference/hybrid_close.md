# styxrpc_hybrid_close

[Русский](hybrid_close.ru.md) · [Declaration](../include/styxrpc/hybrid.h#L50) · [Implementation](../src/hybrid.c) · [Table guide](hybrid_tables.md)

```c
#include <styxrpc/hybrid.h>
void styxrpc_hybrid_close(StyxRpcHybrid *table);
```

Release an owned `table` created by [open](hybrid_open.md) or [stage](hybrid_stage.md), including all copied keys. Passing `NULL` is allowed. Serialize destruction with every operation on the table. The handle expires when the call returns.

[Executable C fixture](../tests/hybrid.c)
