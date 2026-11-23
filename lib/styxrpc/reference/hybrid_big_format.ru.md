# styxrpc_hybrid_big_format

[English](hybrid_big_format.md) · [Объявление](../include/styxrpc/hybrid.h#L81) · [Реализация](../src/hybrid.c) · [Описание таблиц](hybrid_tables.ru.md)

```c
#include <styxrpc/hybrid.h>
size_t styxrpc_hybrid_big_format(int64_t value, char *bytes, size_t capacity);
```

Запишите `value` в каноническом десятичном виде в буфер `bytes` вызывающего. `capacity` измеряется в байтах и должна быть не меньше `STYXRPC_HYBRID_BIG_TEXT` (21). Строка заканчивается нулём. Функция возвращает число символов без завершающего нуля либо ноль при неверном буфере. Память не выделяется.

[Исполняемая проверка C](../tests/hybrid.c)
