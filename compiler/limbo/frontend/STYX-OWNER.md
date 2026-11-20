# limbo_styx_owner

[Русский](STYX-OWNER.ru.md) · [Header](remote_owner.h) · [Implementation](remote_owner.c) · [Example](../../../disVM/examples/remote-scalar/STYX.md)

```c
#include "remote_owner.h"
int limbo_styx_owner(FILE *output, const LimboInterface *catalog, const char *include_name,
    const char *implementation, const char *request, const char *reply,
    const char *request_tag, const char *reply_tag, char *error, size_t capacity);
```

This C99 API in `limbo_remote_owner_lib` emits a standalone Limbo owner for a
single `fn(int): int` export. The borrowed catalog, include basename, absolute
VFS route paths and decimal tags follow the [client generator](STYX-CLIENT.md)
rules. The request must be readable and the reply writable on the owner.
`implementation` names a file visible to the owner's typed `load`, with at most
255 ASCII letters, digits, dots, hyphens, underscores and slashes.

A NULL output preflights the complete request. Return 1 on success or 0 on
invalid input, a name collision or a stream error. The optional diagnostic
buffer has `capacity` bytes including the terminator. The caller retains all
inputs, synchronizes shared streams and handles stream close errors. Invalid
input writes nothing. Stream failures can leave a prefix.

Running the generated Dis program loads the implementation and processes one
valid call on the supplied route. It returns the result or maps an application
exception to error code 1. Generation grants no files and performs no
execution. See the example for mounting and bootstrap.
