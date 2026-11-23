# styxrpc_launch_start

[Русский](launch_start.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/launch.h>
StyxRpcLaunchStatus styxrpc_launch_start(struct StyxClient *client, const void *arguments, size_t size,
    void *scratch, size_t capacity);
```

Borrow the attached client and encode size startup bytes into disjoint scratch. Read the raw launch profile and attempt one confirmed write to .unibus. PENDING confirms exact delivery. UNKNOWN means the run may have started.

Linux64. Each operation may wait for the existing Styx client deadline. `StyxRpc::launch`.

[Example](../examples/disvm/launch.c) · [Source](../src/launch.c)
