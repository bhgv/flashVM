# styxrpc_hybrid_apply

[English](hybrid_apply.md) · [Объявление](../include/styxrpc/hybrid.h#L55) · [Реализация](../src/hybrid.c) · [Описание таблиц](hybrid_tables.ru.md)

```c
#include <styxrpc/hybrid.h>
int styxrpc_hybrid_apply(StyxRpcHybrid *table, StyxRpcHybridRequest *request);
```

Выполните одну операцию над `table` по заимствованному `request`. Установите `operation` в `STYXRPC_HYBRID_GET`, `STYXRPC_HYBRID_SET` или `STYXRPC_HYBRID_DELETE` для обращения к `key`. Числовой ключ задаётся через `text=0` и `index`, отрицательное значение отсчитывается от конца плотной части. Строковый ключ использует `text=1`, `bytes` в UTF-8 и длину `size`. `SET` копирует новый ключ и берёт `value`. `GET` на успехе записывает `value`. `LENGTH` и `COUNT` записывают в `count` плотную длину или общее число записей. Функция возвращает 1 при успехе и 0 при отсутствии ключа, неверном входе или нехватке памяти. Отказ сохраняет таблицу. Выполняйте операции последовательно на владельце.

[Исполняемая проверка C](../tests/hybrid.c)
