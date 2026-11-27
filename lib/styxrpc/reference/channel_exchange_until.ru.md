# styxrpc_channel_exchange_until

[English](channel_exchange_until.md) · [Объявление](../include/styxrpc/channel_call.h#L83) · [Реализация](../src/channel_exchange.c#L214)

```c
#include <styxrpc/channel_call.h>

StyxRpcStatus styxrpc_channel_exchange_until(struct StyxClient *reverse, const StyxRpcChannelCall *call,
    const StyxRpcSelection *selection, unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity,
    StyxRpcChannelResult *result, StyxRpcCallStop stop, void *state);
```

Подключите хостовую цель `StyxRpc::launch`. Операция использует
контракт выбора и маршрута [`styxrpc_channel_exchange`](channel_exchange.ru.md).

Заимствуйте встречное подключение и проверенный вызов с PID вызывающего
и выданными ключами. Удерживайте вызов до завершения операции.

Задайте `timeout_ms` от 1 до 60000 миллисекунд. Передайте раздельные
буферы scratch, request и reply и их размеры в байтах. Scratch нужен
при упаковке веток. При `STYXRPC_OK` выход `result` содержит номер
выбранной ветки и может заимствовать байты элемента из reply.

Необязательный колбэк [`StyxRpcCallStop`](../include/styxrpc/rpc.h)
получает заимствованный `state` в вызывающем потоке. Ненулевое
значение останавливает ожидание. SDK проверяет флаг при обходе и
открытии шины, необходимой аутентификации токена и обмене данными.
В мультиплексированной сессии ожидающий запрос отменяется по тегу.
Подтверждение flush и очистка используют сроки сессии. NULL оставляет
обычное ожидание.

| Исход | Значение |
| --- | --- |
| `STYXRPC_CANCELLED` | Остановка до записи веток. Токен мог быть уже принят при проверке доступа. |
| `STYXRPC_CANCELLED_UNCERTAIN` | Остановка во время или после записи веток. Допущенная ветка могла исполниться. |

В обоих случаях SDK освобождает точный временный fid и сохраняет
`result`. Если ветвь ещё не отправлена, очистка может продолжиться
после возврата. Полностью прочитанный до остановки ответ сохраняет
обычный исход.
Запрос не повторяется. Исходный маршрут остаётся заимствованным.
Таймаут, явный отказ и неверный обмен сохраняют обычные статусы API.
Используйте разные буферы параллельных операций и сериализуйте
изменения общей таблицы маршрутов.

```c
StyxRpcStatus status = styxrpc_channel_exchange_until(
    reverse, call, &selection, 1000, scratch, sizeof(scratch),
    request, sizeof(request), reply, sizeof(reply), &result, stop_requested, task);
```

Здесь `stop_requested(task)` читает флаг остановки вызывающего.
См. [парную проверку](../../../flashVM/doc/STYX_RPC_LIVE_DIS.ru.md)
со сценарием `--cancel-dis-channel`.
