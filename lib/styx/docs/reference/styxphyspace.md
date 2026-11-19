# styxphyspace

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxphyspace.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/server/server.h>

unsigned styxphyspace(Styxserver *server);
```

## Library

Styx::phy_server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `server` | `Styxserver *server` | Original server instance and dispatcher-owned state. |

## Description

styxphyspace: phy/lifetime implementation and native state.

### Preserved implementation/header contract

EN/RU: Owner-only current free reply slots;0 invalid/failed/full, no I/O or permission decision.
Owner-only число свободных reply slots;0 invalid/failed/full, без I/O/permission decision.

## Return value

`unsigned`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::phy_server; hosted native build

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: server/phy/lifetime.c:324](../../server/phy/lifetime.c#L324)
- [Public declaration](../../server/phy/server.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
