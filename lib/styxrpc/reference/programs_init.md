# styxrpc_programs_init

[Русский](programs_init.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/program.h>
int styxrpc_programs_init(StyxRpcPrograms *programs, StyxRpcRegistration *storage, size_t capacity);
```

Initialize fresh storage of 2–4096 slots. Keep both arrays stable through server shutdown.

Linux64. Returns 1 on success, 0 on refusal. Register on the dispatcher thread or before serving. `StyxRpc::serve`.

[Example](../examples/disvm/reverse.c) · [Source](../src/program.c)
