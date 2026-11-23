# styxrpc_program_collection

[Русский](program_collection.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/collection.h>
int styxrpc_program_collection(StyxRpcPrograms *programs, uint32_t pid, const char *name, StyxIntArray *array);
```

Publish .idx, .val, .type and .len and the supported mutations using one fixed-storage registration. Configure a positional or numeric-key Int32 collection. Open .idx and write one big-endian u32 before opening .val on the same connection. Keep .idx open while using .val. Mutations require no active selector. .ins and .add take key/index plus int32, .push takes one int32, .pop reads a per-fid 4-byte result. Keep callback state alive through all opens.

`StyxRpc::serve`.

[Example](../examples/disvm/reverse.c) · [Source](../src/program.c)
