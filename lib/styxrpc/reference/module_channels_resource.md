# styxrpc_module_channels_resource

[Русский](module_channels_resource.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/module_channels.h>
StyxRpcResource styxrpc_module_channels_resource(StyxRpcModuleChannels *channels);
```

Obtain a descriptor for the module's `.unibus`. `channels` is the
initialized registry borrowed by all bus fids. The function returns the
descriptor by value without opening files. Keep the registry and storage
alive while the node is published. The open callback rejects an absent
or uninitialized registry and access other than read/write.

A fresh fid accepts one authentication attempt. Its first write must be
exactly 16 token bytes at offset zero. A rejected write leaves that fid
unusable for operations. On success the backend open runs, if present.
Subsequent writes begin at offset 16 and reach the backend with the token
extent subtracted. Reads are relative to reply offset zero. The backend
sees ordinary positioned reads and writes and owns the operation payload.
The call that builds this descriptor performs no I/O. On use, the
dispatcher allocates one wrapper per successful bus open and invokes
the resource callbacks synchronously.

Library `StyxRpc::serve`.

[Tutorial](../examples/module_channels.md) · [styxrpc_module_channels_revoke](module_channels_revoke.md) · [Source](../src/module_channels.c)
