# styxrpc_hybrid_diff

[English](hybrid_diff.md) · [Объявление](../include/styxrpc/hybrid.h#L65) · [Реализация](../src/hybrid.c) · [Описание таблиц](hybrid_tables.ru.md)

```c
#include <styxrpc/hybrid.h>
int styxrpc_hybrid_diff(const StyxRpcHybrid *table, const void *baseline, size_t size,
    void *bytes, size_t capacity, size_t *used);
```

Сравните текущие данные `table` с полным снимком `baseline` длины `size` и запишите патч Dis в `bytes`. `size` и `capacity` указаны в байтах, максимум 64 КиБ. Функция возвращает 1 и записывает размер патча в `used` либо ноль при совпадении содержимого и порядка строковых ключей. Неверный вход или нехватка места дают 0. Содержимое таблицы должно быть стабильным во время сравнения. Передайте патч, принадлежащий вызывающему, в [stage](hybrid_stage.ru.md).

[Исполняемая проверка C](../tests/hybrid.c)
