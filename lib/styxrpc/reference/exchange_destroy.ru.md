# styxrpc_exchange_destroy

[English](exchange_destroy.md) · [Заголовок](../include/styxrpc/exchange.h) · [Реализация](../src/exchange.c) · [Руководство](exchange.ru.md)

```c
#include <styxrpc/exchange.h>
void styxrpc_exchange_destroy(StyxRpcExchange *exchange);
```

Освобождает собственную очередь и ожидающие данные. NULL допустим. Закройте связанный
клиентский порт или сервер до освобождения очереди. Вызывайте у владельца. I/O не
выполняется.
