# limbo_remote_owner

Copyright (c) 2026 bhgv

[Русский](REMOTE-OWNER.ru.md) · [Tutorial and real-VM test](../../../disVM/examples/remote-scalar/README.md) ·
[Header](remote_owner.h) · [Implementation](remote_owner.c)

```c
#include "remote_owner.h"
int limbo_remote_owner(FILE *output, const LimboInterface *catalog,
                       const char *include_name, const char *implementation,
                       char *error, size_t capacity);
```

Generate the owner-side Proxyhandler adapter for the catalog's single
`fn(int):int` export. This C99 API belongs to `limbo_remote_owner_lib`, which links
the existing `limbo_interfaces`/Coco/R frontend. It neither executes Dis nor
creates a transport, and requires no C++ runtime.

| Parameter | Contract |
| --- | --- |
| output | Borrowed stream; NULL performs full preflight without output |
| catalog | Borrowed immutable parsed interface, alive through return |
| include_name | Borrowed basename of the same interface available to the Dis compiler |
| implementation | Borrowed provider-local path used by the generated ordinary typed load |
| error | Optional caller-owned diagnostic buffer; cleared on entry when capacity is nonzero |
| capacity | Diagnostic buffer bytes including NUL; zero disables writes |

Return1 means supported/generated successfully; return0 means unsupported input,
invalid path/module name, semantic validation failure or stream failure. All
profile validation precedes the first output byte. Stream failures may leave a
prefix. The caller owns stream close, atomic publication if needed, and catalog
retirement. No input pointer is retained.

The accepted profile, path bounds, explicit unsupported operations and executable
example are in the linked tutorial. Immutable catalogs can be read independently;
serialize parsing according to the existing frontend API and synchronize shared
output/error buffers. A compatible interface is not execution permission or
transport authentication. The generated Dis load checks the actual provider
implementation; client-side automatic module binding remains separate work.
