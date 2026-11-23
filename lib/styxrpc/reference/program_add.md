# styxrpc_program_add

[Русский](program_add.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/program.h>
int styxrpc_program_add(StyxRpcPrograms *programs, uint32_t pid);
```

Reserve all five nodes for a nonzero PID. Duplicates and insufficient capacity leave the tree unchanged.

Linux64. Returns 1 on success, 0 on refusal. Register on the dispatcher thread or before serving. `StyxRpc::serve`.

[Example](../examples/disvm/reverse.c) · [Source](../src/program.c)
