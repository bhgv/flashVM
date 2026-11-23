# styxrpc_hybrid_diff

[Русский](hybrid_diff.ru.md) · [Declaration](../include/styxrpc/hybrid.h#L65) · [Implementation](../src/hybrid.c) · [Table guide](hybrid_tables.md)

```c
#include <styxrpc/hybrid.h>
int styxrpc_hybrid_diff(const StyxRpcHybrid *table, const void *baseline, size_t size,
    void *bytes, size_t capacity, size_t *used);
```

Compare current `table` contents with the complete `baseline` snapshot of length `size` and write a Dis key patch into `bytes`. Both `size` and `capacity` are measured in bytes and at most 64 KiB. The function returns 1 and reports the encoded patch extent in `used`, or sets it to zero when contents and text order match. Invalid input or insufficient space returns 0. Keep the table stable during comparison. Use the caller-owned patch with [stage](hybrid_stage.md).

[Executable C fixture](../tests/hybrid.c)
