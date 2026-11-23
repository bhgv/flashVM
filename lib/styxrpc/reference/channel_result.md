# styxrpc_channel_result

[Русский](channel_result.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel.h>
StyxRpcStatus styxrpc_channel_result(const StyxRpcChannelGrant *grants, size_t grant_count,
    const StyxRpcSelection *operation, const void *bytes, size_t size, StyxRpcChannelResult *result);
```

Use the same selection and grants to decode an exact reply. On success publish the ordinal and either an int or a borrowed typed record. REMOTE_ERROR means the peer refused the operation.

C99. `StyxRpc::rpc`.

[Example](../tests/channel_socket.c) · [Source](../src/channel.c)
