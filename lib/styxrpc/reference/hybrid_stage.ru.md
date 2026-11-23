# styxrpc_hybrid_stage

[English](hybrid_stage.md) · [Объявление](../include/styxrpc/hybrid.h#L71) · [Реализация](../src/hybrid.c) · [Описание таблиц](hybrid_tables.ru.md)

```c
#include <styxrpc/hybrid.h>
StyxRpcHybrid *styxrpc_hybrid_stage(const StyxRpcHybrid *table, const void *patch, size_t size);
```

Скопируйте текущее содержимое `table` и примените полный `patch` длиной `size` до 64 КиБ. Меняются только ключи, указанные патчем. Исходная таблица сохраняется. При успехе возвращается независимая таблица, которой можно заменить исходную после проверки остальных частей ответа. Неверный вход или нехватка памяти дают `NULL`. Содержимое должно быть стабильным на время подготовки. [Закройте](hybrid_close.ru.md) старое или отброшенное хранилище.

[Исполняемая проверка C](../tests/hybrid.c)
