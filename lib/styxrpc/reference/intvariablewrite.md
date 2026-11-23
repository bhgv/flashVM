# styxintvariablewrite

[Русский](intvariablewrite.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/variable_file.h>
int styxintvariablewrite(const StyxIntVariable *variable, uint64_t offset, const void *bytes, size_t size);
```

Accept exactly four network-order bytes at offset zero and pass the signed value to store once. Return 1 on acknowledged success or 0 on bad size, bad offset or callback failure.

C99. `StyxRpc::rpc`.

[Example](../examples/disvm/reverse.c) · [Source](../protocol/variable_file.c)
