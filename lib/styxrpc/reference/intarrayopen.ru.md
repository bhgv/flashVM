# styxintarrayopen

[English](intarrayopen.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/indexed_variable.h>
uint64_t styxintarrayopen(StyxIntArray *array, const void *client, int selector);
```

Открывает селектор (ненулевой selector) или связывает значение с текущим поколением (ноль). Client — устойчивый заимствованный идентификатор. Возвращает ненулевое поколение или ноль при отказе.

`StyxRpc::rpc`.

[Пример](../examples/disvm/reverse.c) · [Исходник](../protocol/indexed_variable.c)
