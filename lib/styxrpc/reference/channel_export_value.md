# styxrpc_channel_export_value

[Русский](channel_export_value.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel_call.h>
StyxRpcStatus styxrpc_channel_export_value(const unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE],
    uint32_t key, int unbuffered, void *bytes, size_t capacity, size_t *used);
```

Encode one opaque lease token and a positive 31-bit key as a returned channel value. The unbuffered flag is 0 or 1. The host owns the token registry, checks tokens on the module .unibus, and revokes leases at function clunk.

`StyxRpc::rpc`.

[Example](../examples/disvm/channel_device.c) · [Source](../src/channel_call.c)
