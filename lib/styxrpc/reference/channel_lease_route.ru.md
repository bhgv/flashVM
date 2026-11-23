# styxrpc_channel_lease_route

[English](channel_lease_route.md) · [Объявление](../include/styxrpc/channel_client.h) · [Реализация](../src/channel_client.c) · [Сценарий аренд](channel_leases.ru.md)

Подключите `<styxrpc/channel_client.h>` и линкуйте `StyxRpc::launch`.

```c
const StyxRpcChannelRoute *styxrpc_channel_lease_route(const StyxRpcChannelLease *lease, size_t index);
```

Возвращает заимствованный маршрут только для чтения из `lease` по индексу `index` от нуля. При NULL вместо аренды и при индексе вне допущенных маршрутов возвращает NULL. Выданный адрес действует, пока удерживается эта ссылка на аренду. Функция работает без ввода-вывода на том же сериализованном владельце, что и остальные операции аренды.

Начальные записи соответствуют `StyxRpcLeasedReply.routes`. [Импорт принятого элемента](channel_lease_import_element.ru.md) дописывает новые маршруты. Перебирайте записи до NULL и используйте нужный индекс для [обмена по аренде](channel_lease_exchange.ru.md).
