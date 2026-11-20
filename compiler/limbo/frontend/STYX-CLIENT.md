# limbo_styx_client

[Русский](STYX-CLIENT.ru.md) · [Header](remote_owner.h) · [Implementation](remote_owner.c) · [Tutorial](../../../disVM/examples/remote-scalar/STYX.md)

```c
#include "remote_owner.h"
int limbo_styx_client(FILE *output, const LimboInterface *catalog,
    const char *include_name, const char *request, const char *reply,
    const char *request_tag, const char *reply_tag, char *error, size_t capacity);
```

Available in the C99 `limbo_remote_owner_lib`. The borrowed `catalog` describes
one module exporting one `fn(int): int`, with optional scalar aliases and
constants. `include_name` names the original interface in the generated source,
which also includes `vfsnative.m`. `request` and `reply` name distinct absolute
VFS paths. Paths are limited to 255 ASCII bytes, with letters, digits, dots,
hyphens, underscores and slashes. The include basename follows the same rules
without slashes. Route tags are nonzero decimal strings with a maximum value
of 4294967295.

On success, return 1 and emit a module with the original name and signature to
the borrowed stream. A NULL stream only validates. Return 0 on invalid input,
name collision or write error. The optional `error` buffer has `capacity`
bytes including the terminator. Input refusal precedes any write. A stream
error may leave a prefix. The caller owns the catalog, stream and output
publication and synchronizes any shared stream. The generator performs no
mount, network I/O or Dis execution.
