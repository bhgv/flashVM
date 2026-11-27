# styxrpc_channel_lease_retain

[English](channel_lease_retain.md) · [Объявление](../include/styxrpc/channel_client.h#L38) · [Реализация](../src/channel_client.c) · [Описание аренды](channel_leases.ru.md)

Подключите `StyxRpc::launch`.

```c
#include <styxrpc/channel_client.h>
int styxrpc_channel_lease_retain(StyxRpcChannelLease *lease);
```

Удержите ещё одну ссылку на живую `lease`, например для второго локального объекта. Успех возвращает 1, `NULL` или переполнение счётчика дают 0.

Один владелец последовательно выполняет удержание, обмен и освобождение. Для каждого успешного удержания вызовите [release](channel_lease_release.ru.md). Операция не обращается к транспорту.

[Исполняемая проверка](../examples/disvm/routed_client.c)
