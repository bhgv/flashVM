# phydisconnect

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](phydisconnect.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/phy/phy.h>

void phydisconnect(RemotePhy *phy);
```

## Library

Styx::carrier; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `phy` | `RemotePhy *phy` | Physical-carrier capability; ownership transfer depends on the constructor/operation. |

## Description

phydisconnect: remote_phy/phy implementation and native state.

### Preserved implementation/header contract

EN/RU: Owner disconnects once; destroy/free only after every I/O/wake user retires.
Owner disconnect один раз; free после всех I/O/wake users.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::carrier; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
phydisconnect(mux->phy);
```

[Complete calling context](../../client/mux_loop.c#L432)

## See also

- [Implementation: remote_phy/phy.c:347](../../remote_phy/phy.c#L347)
- [Public declaration](../../remote_phy/phy.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
