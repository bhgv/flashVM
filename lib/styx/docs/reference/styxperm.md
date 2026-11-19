# styxperm

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxperm.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

int styxperm(Styxfile *f, char *uid, int mode);
```

## Library

Styx::server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `f` | `Styxfile *f` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `uid` | `char *uid` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `mode` | `int mode` | Requested operation/permission mode; use the documented protocol constants, not a pathname suffix. |

## Description

styxperm: original server session/fid dispatch and callback-backed namespace operations.

## Return value

`int`; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int styxperm(Styxfile *file, char *uid, int mode);
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-001.inc#L259)

## See also

- [Implementation: vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-002.inc:184](../../vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-002.inc#L184)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
