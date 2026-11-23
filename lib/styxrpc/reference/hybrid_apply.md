# styxrpc_hybrid_apply

[Русский](hybrid_apply.ru.md) · [Declaration](../include/styxrpc/hybrid.h#L55) · [Implementation](../src/hybrid.c) · [Table guide](hybrid_tables.md)

```c
#include <styxrpc/hybrid.h>
int styxrpc_hybrid_apply(StyxRpcHybrid *table, StyxRpcHybridRequest *request);
```

Run one operation on `table` using the borrowed `request`. Set `operation` to `STYXRPC_HYBRID_GET`, `STYXRPC_HYBRID_SET` or `STYXRPC_HYBRID_DELETE` to address one `key`. A numeric key uses `text=0` and `index`. Negative indices address the current dense prefix from the end. A string key uses `text=1`, counted UTF-8 `bytes`, and `size`. `SET` copies a new key and consumes `value`. `GET` writes `value` on success. `LENGTH` and `COUNT` write the dense length or total key count to `count`. The function returns 1 on success or 0 on absence, invalid input or allocation failure. A failed operation leaves the table unchanged. Serialize access on its owner.

[Executable C fixture](../tests/hybrid.c)
