# styxnotifyfd

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxnotifyfd.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

int styxnotifyfd(Styxserver *server, int descriptor);
```

## Library

Styx::phy_server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `server` | `Styxserver *server` | Original server instance and dispatcher-owned state. |
| `descriptor` | `int descriptor` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

styxnotifyfd: phy/backend implementation and native state.

### Preserved implementation/header contract

POSIX borrowed readiness fd: owner registers fd or -1 to remove;0/-1.
Caller drains and closes it after unregister/end. Other ports return -1.
POSIX borrowed fd готовности; owner регистрирует, сам читает и закрывает.

## Return value

`int`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::phy_server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int styxnotifyfd(Styxserver *, int fd);
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-002.inc#L50)

## See also

- [Implementation: server/phy/backend.c:302](../../server/phy/backend.c#L302)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
