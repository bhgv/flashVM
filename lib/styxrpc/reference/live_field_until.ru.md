# styxrpc_live_field_until

[English](live_field_until.md) · [Объявление](../include/styxrpc/live_client.h#L51) · [Реализация](../src/live_collection.c#L245) · [Руководство](../LIVE_VALUES.ru.md)

## Синтаксис

```c
#include <styxrpc/live_client.h>
StyxRpcIoOutcome styxrpc_live_field_until(const StyxRpcLive *live, const char *name,
    StyxRpcLive **out, StyxRpcVfsStop stop, void *state);
```

Подключите `StyxRpc::launch`.

## Параметры и результат

Заимствует запись `live`, одно непустое имя поля `name` без слеша и начальной точки и необязательную функцию `stop(state)` как при [открытии](live_open_until.ru.md). `out` указывает на место для результата. Пути родителя и поля должны укладываться в предел длины. `STYXRPC_IO_OK` выдаёт независимо владеющую привязку поля. Она наследует ограничение записи родителя и права поля. Закрытие родителя сохраняет владение полем. Сессия нужна обеим привязкам.

`STYXRPC_IO_NOT_OPEN` охватывает неверное имя, неверный тип или устаревание родителя и недоступное или неизвестное поле. Синхронный обход может завершиться отменой или сбоем обмена. При отказе `out` не изменяется.


## Пример

```c
StyxRpcLive *name = NULL;
if (styxrpc_live_field_until(details, "name", &name, NULL, NULL) == STYXRPC_IO_OK) {
    /* The child can now be used without retaining details. */
    styxrpc_live_close(name, 0);
}
```

[Полный пример C](../examples/disvm/LIVE_STAGE.ru.md) · [Обмен и владение](../LIVE_VALUES.ru.md#владение-и-ожидание)
