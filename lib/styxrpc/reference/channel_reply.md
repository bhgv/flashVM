# styxrpc_channel_reply

[Русский](channel_reply.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel.h>
StyxRpcStatus styxrpc_channel_reply(const StyxRpcChannelGrant *grants, size_t grant_count,
    const StyxRpcSelection *operation, unsigned selected, const void *value, size_t size,
    void *bytes, size_t capacity, size_t *used);
```

Encode one selected guard, one-based, or an idle (0) or failed (255) outcome. Idle is accepted only for try or bounded wait. Typed receives include a managed element frame. Use disjoint buffers.

C99. `StyxRpc::rpc`.

[Example](../tests/channel_socket.c) · [Source](../src/channel.c)
