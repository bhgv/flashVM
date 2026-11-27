# styxrpc_channel_exchange

[English](channel_exchange.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel_call.h>
StyxRpcStatus styxrpc_channel_exchange(struct StyxClient *reverse, const StyxRpcChannelCall *call,
    const StyxRpcSelection *selection, unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity, StyxRpcChannelResult *result);
```

Выполняет выбор через встречный .unibus проверенного клиента с ключами его вызова. Один отдельный fid на выбор. Буферы не пересекаются, тайм-аут 1–60000 мс. MORE означает истечение времени и попытку clunk. INVALID означает сбой транспорта или формата, REMOTE_ERROR — отказ провайдера. При тайм-ауте и потерянном подтверждении исход может быть неизвестен. Прямой диспетчер должен продолжать отвечать, пока DisVM обслуживает встречные операции.

`StyxRpc::launch`.

[Пример](../examples/disvm/channel_device.c) · [Исходник](../src/channel_exchange.c)

[Операция с отменой](channel_exchange_until.ru.md).
