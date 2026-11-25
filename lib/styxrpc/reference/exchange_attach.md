# styxrpc_exchange_attach

[Русский](exchange_attach.ru.md) · [Header](../include/styxrpc/exchange.h) · [Implementation](../src/exchange.c) · [Guide](exchange.md)

```c
#include <styxrpc/exchange.h>
void styxrpc_exchange_attach(StyxRpcExchange *exchange, StyxRpcMessagePort *port);
```

Associates the client port with a live exchange on its native owner. The exchange
borrows port. Supply NULL to detach, in either case dropping previously queued input.
The server’s lifecycle hooks do this automatically. No Styx requests are sent. The
client owns port close.
