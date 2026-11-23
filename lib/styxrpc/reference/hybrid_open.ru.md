# styxrpc_hybrid_open

[English](hybrid_open.md) · [Объявление](../include/styxrpc/hybrid.h#L45) · [Реализация](../src/hybrid.c) · [Описание таблиц](hybrid_tables.ru.md)

```c
#include <styxrpc/hybrid.h>
StyxRpcHybrid *styxrpc_hybrid_open(const void *snapshot, size_t size);
```

Откройте собственную таблицу из одного полного снимка Dis в `snapshot`. `size` — размер в байтах до 64 КиБ. Пара `NULL, 0` создаёт пустую таблицу. Ключи и значения копируются до возврата. Неверный формат и дубликаты ключей отклоняются. Функция возвращает собственную таблицу или `NULL` при отказе. Освободите её через [close](hybrid_close.ru.md).

[Исполняемая проверка C](../tests/hybrid.c)
