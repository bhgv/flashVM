# Очередь смешанных сообщений

[English](exchange.md) · [UniBus](channel_messages.ru.md)

Очередь и её схемы входят в переносимую библиотеку StyxRpc::rpc. Зарегистрируйте spec в
именованном корне сервера или на клиентском fid через хостовый SDK. Очередь копирует до
1024 полных записей и до двух максимальных размеров данных. Превышение любого лимита
завершает доставляющий порт с ошибкой. Владелец сериализует I/O, вызовы очереди и хуки.
Клиент обслуживает транспорт через [pump](messages_pump.ru.md) или
[flush](messages_flush.ru.md).

- [styxrpc_exchange_create](exchange_create.ru.md)
- [styxrpc_exchange_destroy](exchange_destroy.ru.md)
- [styxrpc_exchange_spec](exchange_spec.ru.md)
- [styxrpc_exchange_attach](exchange_attach.ru.md)
- [styxrpc_exchange_send](exchange_send.ru.md)
- [styxrpc_exchange_pending](exchange_pending.ru.md)
- [styxrpc_exchange_recv](exchange_recv.ru.md)
