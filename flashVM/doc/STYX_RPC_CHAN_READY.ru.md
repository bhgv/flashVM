# rpc.ready

[English](STYX_RPC_CHAN_READY.md) · [Штатная сессия RPC](STYX_RPC_DEFAULT.ru.md) · [Приём](STYX_RPC_CHAN_RECV.ru.md) · [Реализация](../src/styxrpc/session_channel.c)

`rpc.ready()` вызывается без аргументов и возвращает true, если в локальной
очереди есть полная запись, включая Boolean false и null. Метод обходится
без сетевого I/O и сохраняет запись в очереди. Вызывайте из потока владельца
фильма. Лишний аргумент вызывает ошибку аргумента.
