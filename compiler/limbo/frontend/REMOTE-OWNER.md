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

Constants and unqualified scalar aliases may accompany the single function export. Types use the existing lexical C resolver; PATH does not override the explicit implementation path.

## Publish a Styx client with the original ABI

```sh
limbo_remote_owner --styx-client compute.m compute.m /svc/request /svc/reply 17 29 shim.b
```

The first file is the parsed interface. The second argument is the basename
used in the generated include. Supply distinct absolute VFS paths for the
request and reply, followed by their decimal route tags (1 to 4294967295) and
the output source path. Compile the adapter with the unchanged input interface
and `vfsnative.m`. Publish its bytecode at the path used by the ordinary typed
load. Invalid interfaces, invalid paths and out-of-range tags leave existing
output intact. See the [Styx example](../../../disVM/examples/remote-scalar/STYX.md)
for mount setup and test commands.

[C API for this mode](STYX-CLIENT.md)

## Generate the executable Styx owner

`--styx-owner INPUT.m INCLUDE.m IMPLEMENTATION.dis REQUEST REPLY REQUESTTAG REPLYTAG OUTPUT.b`
adds an implementation path to the route arguments used by the Styx client.
Compile the generated source against the original interface. Run it with a
readable request inbox and a writable return route. See
[limbo_styx_owner](STYX-OWNER.md) and the two-DisVM example.
