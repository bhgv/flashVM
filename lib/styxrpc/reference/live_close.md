# styxrpc_live_close

[Русский](live_close.ru.md) · [Declaration](../include/styxrpc/live_client.h#L76) · [Implementation](../src/live_collection.c#L484) · [Guide](../LIVE_VALUES.md)

## Syntax

```c
#include <styxrpc/live_client.h>
void styxrpc_live_close(StyxRpcLive *live, int deferred);
```

Link `StyxRpc::launch`.

## Parameters and effects

Pass the owned `live` pointer, or `NULL`. A zero `deferred` value uses the ordinary clunk path. A nonzero value queues fid retirement so the caller need not wait for a clunk reply. The function returns no status. After it returns, the pointer has been consumed. Clear local aliases. Calling it again with `NULL` is harmless. Use the owner and finish any outstanding operation before closing. Each field binding has independent ownership and must also be closed.


## Example

```c
styxrpc_live_close(binding, 0);
binding = NULL;
```

[Full C example](../examples/disvm/LIVE_STAGE.md) · [Operation contract](../LIVE_VALUES.md#ownership-and-waiting)
