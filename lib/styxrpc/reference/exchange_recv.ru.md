# styxrpc_exchange_recv

[English](exchange_recv.md) · [Заголовок](../include/styxrpc/exchange.h) · [Реализация](../src/exchange.c) · [Руководство](exchange.ru.md)

```c
#include <styxrpc/exchange.h>
StyxRpcStatus styxrpc_exchange_recv(StyxRpcExchange *exchange, StyxRpcExchangeKind *kind,
    void *bytes, size_t capacity, size_t *size);
```

У владельца извлекает старейшее полное сообщение в буфер вызывающего ёмкостью capacity
байт. OK записывает точные kind и size и удаляет запись. MORE означает пустую очередь.
Если запись готова, неверные указатели или недостаточная ёмкость дают INVALID с
сохранением очереди и выходов. Копия принадлежит вызывающему. Запросов Styx нет.
