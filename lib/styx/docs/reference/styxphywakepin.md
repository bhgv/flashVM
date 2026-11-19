# styxphywakepin

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxphywakepin.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/server/server.h>

StyxPhyWake *styxphywakepin(Styxserver *server);
```

## Library

Styx::phy_server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `server` | `Styxserver *server` | Original server instance and dispatcher-owned state. |

## Description

styxphywakepin: phy/lifetime implementation and native state.

### Preserved implementation/header contract

EN/RU: Owner-only pin native wake state while server is live; owned handle/NULL on invalid/overflow.
Producers retain this capability until callbacks retire, including after server end.
Удержать native wake state под server owner; свой handle/NULL, callbacks владеют pin до завершения.

## Return value

`StyxPhyWake *`; direct result constants in this body: nil. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::phy_server; hosted native build

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: server/phy/lifetime.c:49](../../server/phy/lifetime.c#L49)
- [Public declaration](../../server/phy/server.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
