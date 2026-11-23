# styxrpc_channel_call

[Русский](channel_call.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel_call.h>
StyxRpcStatus styxrpc_channel_call(const StyxRpcSchema *schema, const void *bytes, size_t size,
    StyxRpcChannelCall *call);
```

Read one complete scoped graph record and extract its leading PID and admitted channel grants. The original byte buffer stays alive through the call. Aliases must have identical element schemas.

`StyxRpc::rpc`.

[Example](../examples/disvm/channel_device.c) · [Source](../src/channel_call.c)
