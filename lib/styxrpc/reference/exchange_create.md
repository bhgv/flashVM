# styxrpc_exchange_create

[Русский](exchange_create.ru.md) · [Header](../include/styxrpc/exchange.h) · [Implementation](../src/exchange.c) · [Guide](exchange.md)

```c
#include <styxrpc/exchange.h>
StyxRpcExchange *styxrpc_exchange_create(void);
```

Creates an empty disconnected mailbox and returns its owned pointer, or NULL on
allocation failure. It takes no arguments and performs no I/O. Pass the new mailbox to
exchange_spec to register its callbacks. The object must outlive all registered ports.
