# styxrpc_program_unibus

[Русский](program_unibus.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/program.h>
int styxrpc_program_unibus(StyxRpcPrograms *programs, uint32_t pid, const StyxRpcResource *resource);
```

Register one duplex .unibus file under a PID. The resource normally supplies an independent open cookie and retires pending work on close.

Linux64. Returns 1 on success, 0 on refusal. Register on the dispatcher thread or before serving. `StyxRpc::serve`.

[Example](../examples/disvm/reverse.c) · [Source](../src/program.c)
