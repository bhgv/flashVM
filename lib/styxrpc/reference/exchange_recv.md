# styxrpc_exchange_recv

[Русский](exchange_recv.ru.md) · [Header](../include/styxrpc/exchange.h) · [Implementation](../src/exchange.c) · [Guide](exchange.md)

```c
#include <styxrpc/exchange.h>
StyxRpcStatus styxrpc_exchange_recv(StyxRpcExchange *exchange, StyxRpcExchangeKind *kind,
    void *bytes, size_t capacity, size_t *size);
```

On the owner, pops the oldest complete message into the caller’s buffer of capacity
bytes. OK writes the exact kind and size and consumes one entry. MORE means empty. For a
queued record, bad output parameters or insufficient capacity return INVALID and leave
the queue and output intact. The copy remains owned by the caller. This call performs no
wire I/O.
