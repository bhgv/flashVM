# styxrpc_live_kind

[English](live_kind.md) · [Объявление](../include/styxrpc/live_client.h#L44) · [Реализация](../src/live_collection.c#L238) · [Руководство](../LIVE_VALUES.ru.md)

## Синтаксис

```c
#include <styxrpc/live_client.h>
StyxRpcLiveKind styxrpc_live_kind(const StyxRpcLive *live);
```

Подключите `StyxRpc::launch`.

## Параметр и результат

Заимствует действительную привязку `live`, которой ещё владеет вызывающий. Возвращает локальный неизменяемый тип проекции без сетевого обмена. Кода ошибки нет, нужен ненулевой указатель. Тип доступен и после снятия публикации, пока вызывающий владеет привязкой. Операции обмена всё равно сверяют идентичность публикации.

`STYXRPC_LIVE_INT` и `STYXRPC_LIVE_BIG` обозначают скаляры. `STYXRPC_LIVE_INTS` — фиксированный массив знаковых Int32. `STYXRPC_LIVE_BIGS` — вектор `big` или числовая часть гибридной таблицы. `STYXRPC_LIVE_TEXT` — строка с длиной или nil. `STYXRPC_LIVE_ROW` — каталог именованных полей.


## Пример

```c
if (styxrpc_live_kind(binding) == STYXRPC_LIVE_ROW) {
    /* Select a named field before reading a scalar value. */
}
```

[Полный пример C](../examples/disvm/LIVE_STAGE.ru.md) · [Обмен и владение](../LIVE_VALUES.ru.md#владение-и-ожидание)
