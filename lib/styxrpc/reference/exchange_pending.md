# styxrpc_exchange_pending

[Русский](exchange_pending.ru.md) · [Header](../include/styxrpc/exchange.h) · [Implementation](../src/exchange.c) · [Guide](exchange.md)

```c
#include <styxrpc/exchange.h>
int styxrpc_exchange_pending(const StyxRpcExchange *exchange);
```

Returns nonzero on the owner if a complete incoming message is buffered. A NULL exchange
or empty queue returns zero. The call performs no I/O and leaves the queue intact.
