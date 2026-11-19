# styxsetowner

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxsetowner.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

void styxsetowner(char *name);
```

## Library

Styx::server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `name` | `char *name` | Component or schema name in this operation; the function validates its own encoding/length limits. |

## Description

styxsetowner: original server session/fid dispatch and callback-backed namespace operations.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
void styxsetowner(char *);
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-001.inc#L344)

## See also

- [Implementation: vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc:382](../../vendor/inferno/tools/libstyx/readable_parts/styxserver-c/part-005.inc#L382)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
