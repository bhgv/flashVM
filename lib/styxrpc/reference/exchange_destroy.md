# styxrpc_exchange_destroy

[Русский](exchange_destroy.ru.md) · [Header](../include/styxrpc/exchange.h) · [Implementation](../src/exchange.c) · [Guide](exchange.md)

```c
#include <styxrpc/exchange.h>
void styxrpc_exchange_destroy(StyxRpcExchange *exchange);
```

Releases the owned mailbox and queued payloads. NULL is harmless. Close the associated
client port or server before destroying the mailbox. Destruction runs on the owner and
performs no I/O.
