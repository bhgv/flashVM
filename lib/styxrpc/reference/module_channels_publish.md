# styxrpc_module_channels_publish

[Русский](module_channels_publish.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/module_channels.h>
int styxrpc_module_channels_publish(StyxRpcModuleChannels *channels, const StyxRpcResource *resource,
    unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE]);
```

Publish one resource on the initialized module registry `channels`. Supply
`resource` with read and write callbacks and a writable 16-byte `token`
buffer. The descriptor is copied. Its context remains application-owned
through revocation. The optional open and close callbacks manage each
authenticated operation handle.

Returns 1 with the complete token. Store it on the issuing function fid
and include it in the result using the channel export encoder. Returns 0
for invalid arguments, a full registry, issuance counter exhaustion or
entropy failure. On failure the registry and the output token keep their
previous values. If later result encoding fails, revoke the issued token
before returning an error.

Each token combines 12 fresh random bytes and a per-registry issuance
number. Publication may block inside the entropy callback. That callback
must avoid re-entering the registry. Call on the owning dispatcher.

Library `StyxRpc::serve`.

[Tutorial](../examples/module_channels.md) · [styxrpc_module_channels_revoke](module_channels_revoke.md) · [Source](../src/module_channels.c)
