# styxrpc_channel_request

[Русский](channel_request.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel.h>
StyxRpcStatus styxrpc_channel_request(const StyxRpcChannelGrant *grants, size_t grant_count,
    const StyxRpcSelection *operation, void *scratch, size_t scratch_size,
    void *bytes, size_t capacity, size_t *used);
```

Encode a single send/receive or up to eight ordered ALT guards. All keys must be admitted by the immutable grant table. ALT uses a disjoint scratch buffer. The output size is a byte count.

C99. `StyxRpc::rpc`.

[Example](../tests/channel_socket.c) · [Source](../src/channel.c)
