# styxrpc_exchange_spec

[Русский](exchange_spec.ru.md) · [Header](../include/styxrpc/exchange.h) · [Implementation](../src/exchange.c) · [Guide](exchange.md)

```c
#include <styxrpc/exchange.h>
void styxrpc_exchange_spec(StyxRpcExchange *exchange, int hosted,
    StyxRpcMessageType types[STYXRPC_EXCHANGE_END - 1], StyxRpcMessageSpec *spec);
```

Fills the caller’s types array and spec with all version-one mixed schemas. Supply a
live exchange and writable outputs. Client mode uses hosted zero. Server mode uses
hosted nonzero and admits one active fid, discarding its incoming queue when it closes.
Keep the outputs until open or publish has copied them. The callback contexts borrow
exchange until port/server close. Use one registration per mailbox.
