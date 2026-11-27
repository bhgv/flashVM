# styxrpc_live_length_until

[English](live_length_until.md) · [Объявление](../include/styxrpc/live_client.h#L57) · [Реализация](../src/live_collection.c#L269) · [Руководство](../LIVE_VALUES.ru.md)

## Синтаксис

```c
#include <styxrpc/live_client.h>
StyxRpcIoOutcome styxrpc_live_length_until(const StyxRpcLive *live, uint32_t *length,
    StyxRpcVfsStop stop, void *state);
```

Подключите `StyxRpc::launch`.

## Параметры и результат

Передайте заимствованную коллекцию или запись `live` и место `length` для точного счётчика. Необязательная `stop(state)` действует как при [открытии](live_open_until.ru.md). `STYXRPC_IO_OK` читает `.len` провайдера в пределах 0–4 294 967 295. Для массива и вектора это число элементов, для записи число именованных полей, для гибридной таблицы длина плотной части. При следующем обращении длина может измениться.

Скаляр, снятая или заменённая проекция и отсутствие файла длины приводят к отказу. Возможны также повреждённые метаданные, сбой обмена или отмена. При отказе `length` сохраняется. Синхронная операция сама освобождает fid метаданных.


## Пример

```c
uint32_t count;
if (styxrpc_live_length_until(values, &count, NULL, NULL) == STYXRPC_IO_OK && count > 0) {
    /* Position 0 existed when .len was read. */
}
```

[Полный пример C](../examples/disvm/LIVE_STAGE.ru.md) · [Обмен и владение](../LIVE_VALUES.ru.md#владение-и-ожидание)
