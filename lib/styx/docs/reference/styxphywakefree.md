# styxphywakefree

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxphywakefree.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/server/server.h>

void styxphywakefree(StyxPhyWake *context);
```

## Library

Styx::phy_server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `context` | `StyxPhyWake *context` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxphywakefree: phy/lifetime implementation and native state.

### Preserved implementation/header contract

EN/RU: Consume readiness pin without waiting for server/guest; NULL harmless.
Поглотить readiness pin без ожидания server/guest; NULL harmless.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::phy_server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
styxphywakefree(context);
```

[Complete calling context](../../server/phy/lifetime.c#L186)

## See also

- [Implementation: server/phy/lifetime.c:21](../../server/phy/lifetime.c#L21)
- [Public declaration](../../server/phy/server.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
