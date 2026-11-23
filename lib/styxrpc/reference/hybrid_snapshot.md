# styxrpc_hybrid_snapshot

[Русский](hybrid_snapshot.ru.md) · [Declaration](../include/styxrpc/hybrid.h#L60) · [Implementation](../src/hybrid.c) · [Table guide](hybrid_tables.md)

```c
#include <styxrpc/hybrid.h>
int styxrpc_hybrid_snapshot(const StyxRpcHybrid *table, void *bytes, size_t capacity, size_t *used);
```

Encode one complete Dis snapshot of `table` in `bytes`, with `capacity` in the range 1 to 65536 bytes. Numeric keys precede text keys. On success the function returns 1 and sets `used` to the encoded byte count. Failure returns 0. The snapshot is caller-owned and independent from subsequent table changes. Hold the table stable for the duration of the call.

[Executable C fixture](../tests/hybrid.c)
