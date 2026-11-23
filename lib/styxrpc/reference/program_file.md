# styxrpc_program_file

[Русский](program_file.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/program.h>
int styxrpc_program_file(StyxRpcPrograms *programs, uint32_t pid, const char *name, unsigned access,
    uint64_t length, const StyxRpcResource *resource);
```

Register a single named file under .vfs. The modes allow only the specified read/write callbacks.

Linux64. Returns 1 on success, 0 on refusal. Register on the dispatcher thread or before serving. `StyxRpc::serve`.

[Example](../examples/disvm/reverse.c) · [Source](../src/program.c)
