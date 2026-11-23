# styxrpc_launch_named_status

[Русский](launch_named_status.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/launch.h>
StyxRpcLaunchStatus styxrpc_launch_named_status(struct StyxFid *request);
```

Read from the exact fid obtained by launch_named, at byte position zero. PENDING means the Dis provider still has no completion. OK and FAILED confirm program outcome. UNKNOWN indicates a transport failure, invalid fid or malformed result. Repeated observations do not submit another call.

Hosted Linux64. `StyxRpc::launch`.

[Example](../examples/disvm/named.c) · [Source](../src/launch.c)
