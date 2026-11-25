# styxrpc_exchange_pending

[English](exchange_pending.md) · [Заголовок](../include/styxrpc/exchange.h) · [Реализация](../src/exchange.c) · [Руководство](exchange.ru.md)

```c
#include <styxrpc/exchange.h>
int styxrpc_exchange_pending(const StyxRpcExchange *exchange);
```

У владельца возвращает ненулевое значение, если полная входящая запись находится в
очереди. При NULL или пустой очереди возвращает ноль. Вызов без I/O, очередь
сохраняется.
