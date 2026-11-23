# styxrpc_hybrid_big_parse

[Русский](hybrid_big_parse.ru.md) · [Declaration](../include/styxrpc/hybrid.h#L76) · [Implementation](../src/hybrid.c) · [Table guide](hybrid_tables.md)

```c
#include <styxrpc/hybrid.h>
int styxrpc_hybrid_big_parse(const char *bytes, size_t size, int64_t *value);
```

Parse the borrowed `size` bytes from `bytes` as an exact signed 64-bit decimal. An optional leading `+` or `-` is allowed, followed by at least one ASCII digit. Whitespace, decimal points, and overflow are invalid. On success the function returns 1 and writes `value`. It returns 0 without publishing a value on failure. It allocates no memory and does not require a terminator.

[Executable C fixture](../tests/hybrid.c)
