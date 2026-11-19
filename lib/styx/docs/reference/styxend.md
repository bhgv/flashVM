# styxend

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxend.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

char *styxend(Styxserver *server);
```

## Library

Styx::phy_server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `server` | `Styxserver *server` | Original server instance and dispatcher-owned state. |

## Description

Retire the original server, its clients and opened fid state through their cleanup callbacks.

### Preserved implementation/header contract

EN/RU: Original end API retires deferred reads/fids/clients once, then releases native context owner.
Native producer pins remain wakeable after disconnect until their own release.
Исходный end API завершает reads/fids/clients один раз; producer pins остаются wakeable до release.

## Return value

`char *`; direct result constants in this body: nil. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::phy_server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
owner->clean = !initialized || styxend(&server) == nil;
```

[Complete calling context](../../examples/memory_phy/server.c#L71)

## See also

- [Implementation: server/phy/lifetime.c:286](../../server/phy/lifetime.c#L286)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
