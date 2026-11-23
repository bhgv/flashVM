# styxrpc_value_integer

[Русский](value_integer.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/managed.h>
StyxRpcStatus styxrpc_value_integer(unsigned long tag, int64_t value, void *bytes, size_t capacity, size_t *used);
```

Encode one int, big or byte child with its corresponding STYXRPC_VALUE tag. Return INVALID for other tags, bad range or insufficient capacity, leaving used zero.

C99. Serialize operations on the same fid. `StyxRpc::rpc`.

[Example](../examples/adjust.c) · [Source](../src/managed.c)
