# styxintarraywrite

[English](intarraywrite.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/indexed_variable.h>
int styxintarraywrite(StyxIntArray *array, uint64_t generation, uint64_t offset, const void *bytes, size_t size);
```

Записывает ровно четыре знаковых байта в сетевом порядке со смещения ноль в действующем поколении. Один раз вызывает store и сообщает результат.

`StyxRpc::rpc`.

[Пример](../examples/disvm/reverse.c) · [Исходник](../protocol/indexed_variable.c)
