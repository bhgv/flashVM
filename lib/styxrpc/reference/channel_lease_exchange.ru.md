# styxrpc_channel_lease_exchange

[English](channel_lease_exchange.md) · [Объявление](../include/styxrpc/channel_client.h#L48) · [Реализация](../src/channel_client.c) · [Описание аренды](channel_leases.ru.md)

Подключите `StyxRpc::launch`.

```c
#include <styxrpc/channel_client.h>
StyxRpcStatus styxrpc_channel_lease_exchange(StyxRpcChannelLease *lease, size_t route_index,
    const StyxRpcSelection *selection, unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity, StyxRpcChannelResult *result);
```

Выполните операцию на исходном смонтированном модуле `lease` с токеном маршрута `route_index`, считая с нуля в копии маршрутов результата. В `selection` используйте ключи, выданные под этим токеном. Сохраняйте аренду и ветки на время вызова. `timeout_ms` принимает 1–60000. Передайте раздельные рабочий буфер, запрос и ответ с ёмкостями в байтах. Для обёрток веток нужен рабочий буфер.

`STYXRPC_OK` записывает проверенный `result`, при типизированном элементе заимствуя байты ответа. `STYXRPC_MORE` означает истечение срока, `STYXRPC_REMOTE_ERROR` — явный отказ, `STYXRPC_INVALID` — неверный запрос, недоступный маршрут, ошибку транспорта или кадра. Для операции открывается и закрывается новый fid. Время проверяется между синхронными транзакциями. Неопределённый исход передаётся вызывающему без автоматического повтора.

[Исполняемая проверка](../examples/disvm/routed_client.c)

[Операция с отменой](channel_lease_exchange_until.ru.md).
