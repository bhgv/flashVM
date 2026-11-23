# styxrpc_channel_call_reply

[Русский](channel_call_reply.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel_call.h>
StyxRpcStatus styxrpc_channel_call_reply(const StyxRpcSchema *schema, const StyxRpcChannelCall *call,
    const void *result, size_t result_size, const void *updates, size_t update_size,
    void *bytes, size_t capacity, size_t *used);
```

Validate result and sparse updates against the same checked call, then encode one reply. Result and updates are complete S-records and are disjoint from the destination.

`StyxRpc::rpc`.

[Example](../examples/disvm/channel_device.c) · [Source](../src/channel_call.c)
