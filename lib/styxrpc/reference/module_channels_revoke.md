# styxrpc_module_channels_revoke

[Русский](module_channels_revoke.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/module_channels.h>
int styxrpc_module_channels_revoke(StyxRpcModuleChannels *channels,
    const unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE]);
```

Revoke the exact 16-byte `token` in `channels`. Return 1 if the token
was active. Return 0 for an absent token or invalid arguments. This
operation immediately denies new authentication and closes the existing
authenticated handles for that token once. Old fids refuse I/O and later
clunk releases their wrapper. The released slot can host a new token.
Other tokens continue to serve their own handles.

Call from the issuing function's close handler and on errors after
publication. The dispatcher runs each backend close inline, so any
blocking depends on those callbacks. If a backend uses
`styxrpc_channel_fid_close`, pending scheduler operations are cancelled.
Committed operations retain their effects. The application can release
the resource context after return. Call outside callbacks of the
target resource.

Library `StyxRpc::serve`.

[Tutorial](../examples/module_channels.md) · [styxrpc_module_channels_publish](module_channels_publish.md) · [Source](../src/module_channels.c)
