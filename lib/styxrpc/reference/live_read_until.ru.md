# styxrpc_live_read_until

[English](live_read_until.md) · [Объявление](../include/styxrpc/live_client.h#L62) · [Реализация](../src/live_collection.c#L380) · [Руководство](../LIVE_VALUES.ru.md)

## Синтаксис

```c
#include <styxrpc/live_client.h>
StyxRpcIoOutcome styxrpc_live_read_until(const StyxRpcLive *live, int64_t index,
    StyxRpcLiveValue *out, StyxRpcVfsStop stop, void *state);
```

Подключите `StyxRpc::launch`.

## Параметры

Заимствует `live`. Для скаляра и строки укажите `index` равным нулю. Фиксированный массив Int32 принимает неотрицательный индекс меньше текущей длины. Вектор `big` принимает знаковую позицию, включая `-1` для последнего элемента. Гибридная таблица принимает точный знаковый числовой ключ. Отрицательный индекс отсчитывается от конца её плотной части. `out` указывает на место для результата. Необязательная `stop(state)` действует как при [открытии](live_open_until.ru.md).

## Результаты и время жизни

`STYXRPC_IO_OK` выдаёт нативное знаковое 64-битное `out->integer`, а для строки `out->nil` и `out->size` байт UTF-8 в `out->text`. Завершающий ноль не гарантирован. Нулевая длина с `nil == 0` означает пустую строку. Встроенный ноль допустим. Результат представляет собой копию.

Отмена, ошибка или отказ сохраняют `*out`. Неверный индекс или тип узла, отсутствие ячейки или устаревшая публикация могут привести к отказу. Повреждённый кадр целиком отклоняется. Временные fid значения и селектора освобождаются и при отказе. Владелец сериализует обмен по соединению.


## Пример

```c
StyxRpcLiveValue sample;
if (styxrpc_live_read_until(values, 0, &sample, NULL, NULL) == STYXRPC_IO_OK) {
    /* sample.integer now holds the selected signed value. */
}
```

[Полный пример C](../examples/disvm/LIVE_STAGE.ru.md) · [Обмен и владение](../LIVE_VALUES.ru.md#владение-и-ожидание)
