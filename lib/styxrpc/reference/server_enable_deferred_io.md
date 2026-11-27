# styxrpc_server_enable_deferred_io

[Русский](server_enable_deferred_io.ru.md) · [Provider guide](../DEFERRED_IO.md) · [Create a server](server_create.md)

## Syntax

```c
#include <styxrpc/serve.h>

typedef unsigned (*StyxRpcDeferFile)(void *context, uint64_t id);

int styxrpc_server_enable_deferred_io(StyxRpcServer *server, StyxRpcDeferFile select);
```

Hosted C99, target `StyxRpc::serve`.

## Parameters and result

| Parameter | Meaning |
| --- | --- |
| `server` | Ordinary callback-tree server. Call on its owner before the first dispatch. |
| `select` | Required borrowed callback. Keep it and the server's tree context valid through destruction. The owner passes the original `context` and the exact ordinary file `id` when opening or creating a file. |

The selector returns a mask of `STYXRPC_READ` and `STYXRPC_WRITE`. The server
intersects it with the fid's admitted access and stores the selection for that
fid. Zero uses synchronous callbacks.

Returns `1` after registration. Invalid arguments, service-only adapters,
repeated registration, dispatch already started or failure to obtain an owner
wake pin give `0`.

## Ownership and cancellation

Selected callbacks run as ordinary `StyxRpcVfs.read` and `StyxRpcVfs.write`
on native workers. The server borrows each cookie through worker completion
and owns the transfer buffers until encoding the response. One active or
retiring operation is allowed per fid. The per-server limit is sixteen.
Additional requests at the limit receive `Rerror`. The owner runs the
selector, opens and closes, metadata, mutations and unselected transfers.
Synchronize shared provider state that those callbacks access concurrently.

Check [styxrpc_server_io_cancelled](server_io_cancelled.md) during a blocked worker
callback. A retired request suppresses its late result. An acknowledged count
confirms only the bytes that the provider reported. Destroy first signals stop,
then joins the workers and calls the remaining file closers.

## Example

With an ordinary server and one thread-safe byte-file ID, install this selection.

```c
static unsigned select_file(void *context, uint64_t id)
{
    (void)context;
    return id == 2 ? STYXRPC_READ | STYXRPC_WRITE : 0;
}
```

Call `styxrpc_server_enable_deferred_io(server, select_file)` before the step
loop. The [runnable provider](../examples/disvm/deferred_device.c) includes
the same selection, worker waits and a synchronous control file. Run it with
the [test guide](../examples/disvm/DEFERRED_IO.md).

[Header](../include/styxrpc/serve.h) · [Implementation](../src/serve_deferred_io.inc)
