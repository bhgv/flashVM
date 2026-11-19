# phywake

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](phywake.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/phy/phy.h>

void phywake(RemotePhy *phy);
```

## Library

Styx::carrier; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `phy` | `RemotePhy *phy` | Physical-carrier capability; ownership transfer depends on the constructor/operation. |

## Description

phywake: remote_phy/phy implementation and native state.

### Preserved implementation/header contract

EN/RU: Nonwaiting native interrupt while lifetime pinned; NULL harmless, no wire traffic.
Native interrupt под lifetime pin; NULL harmless, без wire traffic.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::carrier; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
phywake(phy);
```

[Complete calling context](../../tests/memory_contract.c#L96)

## See also

- [Implementation: remote_phy/phy.c:254](../../remote_phy/phy.c#L254)
- [Public declaration](../../remote_phy/phy.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
