# styxrpc_exchange_send

[English](exchange_send.md) · [Заголовок](../include/styxrpc/exchange.h) · [Реализация](../src/exchange.c) · [Руководство](exchange.ru.md)

```c
#include <styxrpc/exchange.h>
StyxRpcStatus styxrpc_exchange_send(StyxRpcExchange *exchange, StyxRpcExchangeKind kind,
    const void *bytes, size_t size);
```

Ставит копию size байт для kind в очередь активного порта. Передайте живую очередь и
известный вид сообщения. size ограничен 16384 байтами. NULL в bytes допустим при нулевой
длине. OK подтверждает очередь, MORE означает отсутствие порта или места, INVALID
означает ошибку параметров или порта. Вызывайте у владельца. Байты позднее переносит
транспорт.
