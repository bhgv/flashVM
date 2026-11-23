# styxrpc_program_variable

[Русский](program_variable.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/program.h>
int styxrpc_program_variable(StyxRpcPrograms *programs, uint32_t pid, const char *name, unsigned access,
    uint64_t length, const StyxRpcResource *resource);
```

Register a byte-file variable under .var. The application defines its encoding; use program_int for signed Int32 interop.

Linux64. Returns 1 on success, 0 on refusal. Register on the dispatcher thread or before serving. `StyxRpc::serve`.

[Example](../examples/disvm/reverse.c) · [Source](../src/program.c)
