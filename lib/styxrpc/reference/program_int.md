# styxrpc_program_int

[Русский](program_int.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/program.h>
int styxrpc_program_int(StyxRpcPrograms *programs, uint32_t pid, const char *name, const StyxIntVariable *variable);
```

Register a named signed 32-bit variable. The copied descriptor supplies independent load/store callbacks, at least one must exist.

Linux64. Returns 1 on success, 0 on refusal. Register on the dispatcher thread or before serving. `StyxRpc::serve`.

[Example](../examples/disvm/reverse.c) · [Source](../src/program.c)
