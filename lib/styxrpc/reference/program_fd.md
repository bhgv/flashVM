# styxrpc_program_fd

[Русский](program_fd.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/program.h>
int styxrpc_program_fd(StyxRpcPrograms *programs, uint32_t pid, unsigned fd, unsigned access,
    uint64_t length, const StyxRpcResource *resource);
```

Register FD 0–31 for an existing PID. The access bits, byte length and borrowed callbacks describe one delegated resource.

Linux64. Returns 1 on success, 0 on refusal. Register on the dispatcher thread or before serving. `StyxRpc::serve`.

[Example](../examples/disvm/reverse.c) · [Source](../src/program.c)
