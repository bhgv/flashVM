# styxfindfile

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxfindfile.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

Styxfile *styxfindfile(Styxserver *server, Path path);
```

## Library

Styx::server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `server` | `Styxserver *server` | Original server instance and dispatcher-owned state. |
| `path` | `Path path` | Selected object path or stable identity according to the declared type; no implicit authority is granted. |

## Description

styxfindfile: original server session/fid dispatch and callback-backed namespace operations.

## Return value

`Styxfile *`; direct result constants in this body: nil. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
Styxfile *styxfindfile(Styxserver *server, Path qid);
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-001.inc#L242)

## See also

- [Implementation: vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-002.inc:259](../../vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-002.inc#L259)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
