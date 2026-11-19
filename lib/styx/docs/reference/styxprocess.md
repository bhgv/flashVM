# styxprocess

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxprocess.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

char *styxprocess(Styxserver *server);
```

## Library

Styx::phy_server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `server` | `Styxserver *server` | Original server instance and dispatcher-owned state. |

## Description

Dispatch available original Styx messages through the selected server callbacks.

### Preserved implementation/header contract

EN/RU: Original process API scopes owner-local close tokens; hardware output remains nonblocking.
Исходный process API задаёт owner-local close context; hardware output nonblocking.

## Return value

`char *`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::phy_server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
styxprocess(&server);
```

[Complete calling context](../../vendor/inferno/tools/libstyx/tests/session.c#L446)

## See also

- [Implementation: server/phy/lifetime.c:214](../../server/phy/lifetime.c#L214)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
