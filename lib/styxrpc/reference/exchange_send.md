# styxrpc_exchange_send

[Русский](exchange_send.ru.md) · [Header](../include/styxrpc/exchange.h) · [Implementation](../src/exchange.c) · [Guide](exchange.md)

```c
#include <styxrpc/exchange.h>
StyxRpcStatus styxrpc_exchange_send(StyxRpcExchange *exchange, StyxRpcExchangeKind kind,
    const void *bytes, size_t size);
```

Queues a copy of size payload bytes for kind on the active port. Supply a live exchange
and a known kind, with size up to 16384. A NULL bytes pointer is allowed for zero size.
OK means queued. MORE means no port or insufficient queue room, and INVALID means bad
input or a failed port. Call on the owner. The underlying transport subsequently moves
queued bytes.
