# styxrpc_cell_value

[Русский](cell_value.ru.md) · [Declaration](../include/styxrpc/named_cell.h#L30) · [Implementation](../src/named_cell_wire.c#L70) · [Cells](named_cell.md)

```c
#include <styxrpc/named_cell.h>

int styxrpc_cell_value(const void *frame, size_t size);
```

Pass a borrowed readable frame and its length in bytes, including the leading
named type byte. The function returns one for a [complete scalar](cell_scalar.md)
or a graph frame with a valid FVMG signature, version (1–9), declared body length
and IEEE CRC-32. It returns zero for a null frame, an empty frame, a frame larger
than 65,536 bytes, a descriptor, or a failed envelope check. The receiving VM
validates the graph contents and decides whether it can deserialize the value.

Link `StyxRpc::rpc`. Validation performs no I/O or allocation, changes no state
and never retains the caller's buffer. Use it before cell creation or replacement
to check a frame received from another component.
