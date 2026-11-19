# styxphywake

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxphywake.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/server/server.h>

void styxphywake(StyxPhyWake *context);
```

## Library

Styx::phy_server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `context` | `StyxPhyWake *context` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxphywake: phy/lifetime implementation and native state.

### Preserved implementation/header contract

EN/RU: Signal native I/O readiness through an owned wake pin; no wire traffic or server execution.
Сигнал native I/O readiness под owned wake pin, без wire traffic/server execution.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::phy_server; hosted native build

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: server/phy/lifetime.c:81](../../server/phy/lifetime.c#L81)
- [Public declaration](../../server/phy/server.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
