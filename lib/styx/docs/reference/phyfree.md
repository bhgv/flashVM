# phyfree

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](phyfree.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/phy/phy.h>

void phyfree(RemotePhy *phy);
```

## Library

Styx::carrier; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `phy` | `RemotePhy *phy` | Physical-carrier capability; ownership transfer depends on the constructor/operation. |

## Description

phyfree: remote_phy/phy implementation and native state.

### Preserved implementation/header contract

EN/RU: Consume owned carrier after all I/O/wake callers retire; NULL harmless.
Поглотить owned carrier после всех I/O/wake callers; NULL harmless.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::carrier; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
phyfree(client_phy);
```

[Complete calling context](../../examples/memory_phy/main.c#L78)

## See also

- [Implementation: remote_phy/phy.c:370](../../remote_phy/phy.c#L370)
- [Public declaration](../../remote_phy/phy.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
