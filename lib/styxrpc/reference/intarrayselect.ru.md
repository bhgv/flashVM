# styxintarrayselect

[English](intarrayselect.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/indexed_variable.h>
int styxintarrayselect(StyxIntArray *array, uint64_t generation, uint64_t offset, const void *bytes, size_t size);
```

Записывает ровно четыре байта ключа в сетевом порядке со смещения ноль в активный селектор. Перед выбором проверяет наличие позиции или ключа. Возвращает 1 при успехе, иначе 0.

`StyxRpc::rpc`.

[Пример](../examples/disvm/reverse.c) · [Исходник](../protocol/indexed_variable.c)
