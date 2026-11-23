# styxintvariableread

[Русский](intvariableread.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/variable_file.h>
int styxintvariableread(const StyxIntVariable *variable, uint64_t offset, void *bytes, size_t *size);
```

Read one complete signed 32-bit value using the borrowed load callback. On entry size is the byte capacity. On success, size is four network-order bytes or zero for EOF at offset four or later. A zero-capacity request returns zero bytes. Other offsets, insufficient space and callback failure return 0. The host serializes access to its own state.

C99. `StyxRpc::rpc`.

[Example](../examples/disvm/reverse.c) · [Source](../protocol/variable_file.c)
