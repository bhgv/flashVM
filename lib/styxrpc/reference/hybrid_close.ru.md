# styxrpc_hybrid_close

[English](hybrid_close.md) · [Объявление](../include/styxrpc/hybrid.h#L50) · [Реализация](../src/hybrid.c) · [Описание таблиц](hybrid_tables.ru.md)

```c
#include <styxrpc/hybrid.h>
void styxrpc_hybrid_close(StyxRpcHybrid *table);
```

Освободите `table`, полученную через [open](hybrid_open.ru.md) или [stage](hybrid_stage.ru.md), вместе с копиями ключей. Допускается `NULL`. Выполняйте уничтожение последовательно с другими операциями над таблицей. После возврата дескриптор недействителен.

[Исполняемая проверка C](../tests/hybrid.c)
