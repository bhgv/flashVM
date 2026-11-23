# styxrpc_launch_status

[Русский](launch_status.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/launch.h>
StyxRpcLaunchStatus styxrpc_launch_status(struct StyxClient *client);
```

Read .exit after a confirmed submission. Return PENDING while the result is empty, OK or FAILED for a confirmed receipt and UNKNOWN on link loss or invalid data. No automatic replay.

Linux64. Each operation may wait for the existing Styx client deadline. `StyxRpc::launch`.

[Example](../examples/disvm/launch.c) · [Source](../src/launch.c)
