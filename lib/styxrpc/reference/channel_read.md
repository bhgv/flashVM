# styxrpc_channel_read

[Русский](channel_read.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel.h>
StyxRpcStatus styxrpc_channel_read(const StyxRpcChannelGrant *grants, size_t grant_count,
    const void *bytes, size_t size, StyxRpcSelection *operation);
```

Decode one complete operation, validate all keys and element records, and return borrowed guards. Keep the request bytes alive while using the decoded selection.

C99. `StyxRpc::rpc`.

[Example](../tests/channel_socket.c) · [Source](../src/channel.c)
