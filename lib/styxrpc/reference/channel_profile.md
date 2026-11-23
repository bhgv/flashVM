# styxrpc_channel_profile

[Русский](channel_profile.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel_call.h>
StyxRpcStatus styxrpc_channel_profile(const char *name, uint32_t signature, const StyxRpcSchema *schema,
    void *bytes, size_t capacity, size_t *used);
```

Write one scoped .interface record from a compiled Limbo signature. Publish .unibus-version and the positional .ref-fields marker for the module. Use a bidirectional Styx mount.

`StyxRpc::rpc`.

[Example](../examples/disvm/channel_device.c) · [Source](../src/channel_call.c)
