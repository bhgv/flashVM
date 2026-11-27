# styxrpc_live_write_until

[English](live_write_until.md) · [Объявление](../include/styxrpc/live_client.h#L70) · [Реализация](../src/live_collection.c#L437) · [Руководство](../LIVE_VALUES.ru.md)

## Синтаксис

```c
#include <styxrpc/live_client.h>
StyxRpcIoOutcome styxrpc_live_write_until(const StyxRpcLive *live, int64_t index,
    const StyxRpcLiveValue *value, StyxRpcVfsStop stop, void *state);
```

Подключите `StyxRpc::launch`.

## Параметры

Заимствует записываемую привязку `live` и новое значение `value`. Индекс задаётся как при [чтении](live_read_until.ru.md). Для числа заполните `value->integer`. Int32 допускает только знаковый 32-битный диапазон. Для текста укажите `nil` равным 0, корректный UTF-8 в `text` и длину `size` до 1024 байт. Для nil укажите `nil` равным 1 и нулевой размер. Конечный ноль не нужен. `stop` и `state` действуют как при [открытии](live_open_until.ru.md).

## Результаты и эффект

`STYXRPC_IO_OK` подтверждает одну полную запись со смещения ноль. На этапе допуска выявляются устаревшая привязка, режим только для чтения, недопустимые индекс или значение. `STYXRPC_IO_CANCELLED` может прервать подготовку. `STYXRPC_IO_UNCERTAIN` означает, что владелец мог принять значение. Сверьте ту же действующую привязку или состояние приложения. SDK не досылает остаток и не повторяет запись после ошибки или неопределённого исхода. Вызывайте у владельца сессии с сериализацией операций.


## Пример

```c
StyxRpcLiveValue replacement = {0};
replacement.integer = 42;
StyxRpcIoOutcome status = styxrpc_live_write_until(values, 0, &replacement, NULL, NULL);
/* A retry is a separate application decision after reconciliation. */
```

[Полный пример C](../examples/disvm/LIVE_STAGE.ru.md) · [Обмен и владение](../LIVE_VALUES.ru.md#владение-и-ожидание)
