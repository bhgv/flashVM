# styxrpc_launch_named

[Русский](launch_named.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/launch.h>
StyxRpcLaunchStatus styxrpc_launch_named(struct StyxClient *client, const char *name,
    const StyxRpcProgram *program, const void *arguments, size_t size,
    void *scratch, size_t capacity, struct StyxFid **request);
```

Use an attached bidirectional Styx client. Supply a single public basename ending in `.dis`, at most 31 bytes. The caller already publishes the nonzero `program->pid` and any distinct parent PID under `.prog` on the reverse stream. `hidden_fds` masks inherited descriptors by number. `arguments` carries `size` bytes, including NULs. The full encoded record must fit both the disjoint scratch buffer and the provider’s 64-byte frame limit. Initialize `*request` to NULL. PENDING transfers one fid to the caller. Before closing it, preserve the reverse publication. NOT_SENT and UNKNOWN retain no fid. UNKNOWN can mean the program started and its cancellation or outcome could not be confirmed.

Hosted Linux64. `StyxRpc::launch`.

[Example](../examples/disvm/named.c) · [Source](../src/launch.c)
