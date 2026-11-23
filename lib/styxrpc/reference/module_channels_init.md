# styxrpc_module_channels_init

[Русский](module_channels_init.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/module_channels.h>
int styxrpc_module_channels_init(StyxRpcModuleChannels *channels, StyxRpcModuleChannel *slots,
    size_t capacity, int (*entropy)(void *, void *, size_t), void *context);
```

Initialize the hosted registry for one mounted module on one Styx attachment.

| Parameter | Meaning |
| --- | --- |
| `channels` | Caller-owned registry object to initialize. Keep its address stable. |
| `slots` | Writable array of `capacity` lease slots. Keep it stable until server shutdown. |
| `capacity` | Maximum simultaneous leases, in slots. Must be positive and fit the supplied array. |
| `entropy` | Callback that fills all requested bytes with cryptographically secure random data and returns 1, or returns 0 on failure. |
| `context` | Optional state passed to the entropy callback. It must live until the last publication. |

Returns 1 after clearing the slots and setting up the registry. Returns 0 for
a null registry, storage or entropy callback, zero capacity or an overflowing
storage size. Initialization performs no I/O, allocation or entropy reads.
Call before serving. A registry and its slots must stay intact until all
open fids have closed. Reinitialization starts a fresh token namespace.
All later operations run on the owning dispatcher.

Library `StyxRpc::serve`.

[Tutorial](../examples/module_channels.md) · [styxrpc_module_channels_publish](module_channels_publish.md) · [Source](../src/module_channels.c)
