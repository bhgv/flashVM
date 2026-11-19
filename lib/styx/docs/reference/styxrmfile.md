# styxrmfile

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxrmfile.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

int styxrmfile(Styxserver *server, Path qid);
```

## Library

Styx::server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `server` | `Styxserver *server` | Original server instance and dispatcher-owned state. |
| `qid` | `Path qid` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

styxrmfile: original server session/fid dispatch and callback-backed namespace operations.

## Return value

`int`; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
styxrmfile(&s, 3);
```

[Complete calling context](../../vendor/inferno/tools/libstyx/tests/directories.c#L302)

## See also

- [Implementation: vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-002.inc:47](../../vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-002.inc#L47)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
