# styxrpc_hybrid_snapshot

[English](hybrid_snapshot.md) · [Объявление](../include/styxrpc/hybrid.h#L60) · [Реализация](../src/hybrid.c) · [Описание таблиц](hybrid_tables.ru.md)

```c
#include <styxrpc/hybrid.h>
int styxrpc_hybrid_snapshot(const StyxRpcHybrid *table, void *bytes, size_t capacity, size_t *used);
```

Запишите полный снимок Dis таблицы `table` в `bytes` с ёмкостью `capacity` от 1 до 65536 байт. Числовые ключи предшествуют строковым. При успехе функция возвращает 1 и записывает размер в байтах в `used`. При отказе возвращает 0. Снимок принадлежит вызывающему и независим от последующих изменений. Содержимое таблицы должно быть стабильным на время вызова.

[Исполняемая проверка C](../tests/hybrid.c)
