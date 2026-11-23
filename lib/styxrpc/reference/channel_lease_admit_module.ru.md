# styxrpc_channel_lease_admit_module

[English](channel_lease_admit_module.md) · [Объявление](../include/styxrpc/channel_client.h) · [Реализация](../src/channel_client.c) · [Сценарий аренд](channel_leases.ru.md)

Подключите `<styxrpc/channel_client.h>` и линкуйте `StyxRpc::launch`.

```c
StyxRpcStatus styxrpc_channel_lease_admit_module(StyxRpcChannelLease *lease,
    const StyxRpcChannelRoute *route, size_t *index);
```

Передайте живую аренду `lease`, разрешённый адрес `STYXRPC_ROUTE_MODULE` в `route` и ненулевой указатель `index`. Хост подтверждает, что адрес относится к этой конкретной выдавшей аренде. Для маршрута из сообщения сначала проверьте транспорт, схему элемента и адрес. SDK копирует адрес и привязывает операции к исходному удерживаемому модулю. Допуск работает без нового поиска пути и сетевого ввода-вывода.

`STYXRPC_OK` записывает существующий или новый индекс маршрута от нуля. Равные пары токен–ключ объединяются при совпадении типа и ёмкости. Неверный вход, конфликт или исчерпание восьми записей дают `STYXRPC_INVALID`, сохраняя таблицу и индекс. Используйте сериализованного владельца аренды.

Передавайте индекс [обмену по аренде](channel_lease_exchange.ru.md). Операция использует новый fid, предъявляет точный токен и освобождает fid при завершении. Отзыв определяется clunk выдавшей функции. [Нативный сценарий C](../examples/disvm/routed_client.c)
