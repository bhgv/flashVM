# styxphypending

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxphypending.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/server/server.h>

size_t styxphypending(Styxserver *server);
```

## Library

Styx::phy_server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `server` | `Styxserver *server` | Original server instance and dispatcher-owned state. |

## Description

styxphypending: phy/lifetime implementation and native state.

### Preserved implementation/header contract

EN/RU: Owner-only unsent encoded bytes,0 invalid/empty; never interprets queued contents.
Owner-only unsent bytes,0 invalid/empty, без интерпретации contents.

## Return value

`size_t`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::phy_server; hosted native build

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: server/phy/lifetime.c:350](../../server/phy/lifetime.c#L350)
- [Public declaration](../../server/phy/server.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
