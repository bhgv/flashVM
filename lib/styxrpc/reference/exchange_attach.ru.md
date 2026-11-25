# styxrpc_exchange_attach

[English](exchange_attach.md) · [Заголовок](../include/styxrpc/exchange.h) · [Реализация](../src/exchange.c) · [Руководство](exchange.ru.md)

```c
#include <styxrpc/exchange.h>
void styxrpc_exchange_attach(StyxRpcExchange *exchange, StyxRpcMessagePort *port);
```

Связывает клиентский порт с живой очередью у её нативного владельца. Очередь заимствует
port. Передайте NULL для отсоединения. В обоих случаях накопленный вход сбрасывается.
Серверные хуки делают это автоматически. Запросов Styx нет. Клиент владеет закрытием
порта.
