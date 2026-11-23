# styxrpc_channel_import_element

[Русский](channel_import_element.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel_call.h>
StyxRpcStatus styxrpc_channel_import_element(StyxRpcChannelCall *call, uint32_t via,
    const void *bytes, size_t size);
```

Call after a successful typed receive to admit the nested keys carried by one element.
`call` is the active call on the original reverse Styx attachment. `via` is the
admitted receive key, whose registered element type supplies the schema. Pass the
complete `bytes` and byte `size` from the receive result. The original call request
must remain alive. The function copies key and type descriptors and retains no
pointer to the received element.

`STYXRPC_OK` adds distinct call-local keys and coalesces aliases of the same type.
`STYXRPC_INVALID` leaves the scope unchanged. Causes include malformed records,
inconsistent aliases, other-route grant records and exhaustion of the
`STYXRPC_CHANNEL_SLOTS` table. Treat only confirmed receives on the same live
route as authority for import. Close the call to end use of its grants.

Link `StyxRpc::rpc`. The function is synchronous and performs no I/O. Serialize
access to an individual call record.

[styxrpc_channel_exchange](channel_exchange.md) · [Example](../examples/disvm/channel_device.c) · [Source](../src/channel_call.c)
