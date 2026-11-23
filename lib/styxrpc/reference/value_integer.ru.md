# styxrpc_value_integer

[English](value_integer.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/managed.h>
StyxRpcStatus styxrpc_value_integer(unsigned long tag, int64_t value, void *bytes, size_t capacity, size_t *used);
```

Кодирует int, big или byte с соответствующим тегом STYXRPC_VALUE. Остальные теги, неверный диапазон и недостаточная ёмкость дают INVALID и нулевой used.

C99. Сериализуйте операции одного fid. `StyxRpc::rpc`.

[Пример](../examples/adjust.c) · [Исходник](../src/managed.c)
