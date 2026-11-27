# styxrpc_live_close

[English](live_close.md) · [Объявление](../include/styxrpc/live_client.h#L76) · [Реализация](../src/live_collection.c#L484) · [Руководство](../LIVE_VALUES.ru.md)

## Синтаксис

```c
#include <styxrpc/live_client.h>
void styxrpc_live_close(StyxRpcLive *live, int deferred);
```

Подключите `StyxRpc::launch`.

## Параметры и эффект

Передайте собственный указатель `live` или `NULL`. При нулевом `deferred` выполняется обычное закрытие fid. Ненулевое значение ставит освобождение fid в очередь, чтобы вызывающий не ожидал ответа clunk. Кода результата нет. После возврата указатель поглощён. Сбросьте локальные алиасы. Повторный вызов с `NULL` допустим. Вызывайте у владельца после завершения операций над привязкой. Дочерние поля владеют собственными fid и закрываются отдельно.


## Пример

```c
styxrpc_live_close(binding, 0);
binding = NULL;
```

[Полный пример C](../examples/disvm/LIVE_STAGE.ru.md) · [Обмен и владение](../LIVE_VALUES.ru.md#владение-и-ожидание)
