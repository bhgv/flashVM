# styxaddfile

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxaddfile.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

Styxfile *styxaddfile(Styxserver *server, Path pqid, Path qid, char *name, int mode, char *owner);
```

## Library

Styx::server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `server` | `Styxserver *server` | Original server instance and dispatcher-owned state. |
| `pqid` | `Path pqid` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `qid` | `Path qid` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `name` | `char *name` | Component or schema name in this operation; the function validates its own encoding/length limits. |
| `mode` | `int mode` | Requested operation/permission mode; use the documented protocol constants, not a pathname suffix. |
| `owner` | `char *owner` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxaddfile: original server session/fid dispatch and callback-backed namespace operations.

## Return value

`Styxfile *`; direct result constants in this body: nil. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
check(styxaddfile(&s, Qroot, 3, "hidden", 0444, "alice") != nil, "hidden cached child");
```

[Complete calling context](../../vendor/inferno/tools/libstyx/tests/directories.c#L290)

## See also

- [Implementation: vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc:264](../../vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc#L264)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
