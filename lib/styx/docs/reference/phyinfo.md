# phyinfo

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](phyinfo.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/phy/phy.h>

int phyinfo(RemotePhy *phy, PhyInfo *out);
```

## Library

Styx::carrier; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `phy` | `RemotePhy *phy` | Physical-carrier capability; ownership transfer depends on the constructor/operation. |
| `out` | `PhyInfo *out` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |

## Description

phyinfo: remote_phy/phy implementation and native state.

### Preserved implementation/header contract

EN/RU: Copy local capabilities/lifecycle;1/0, owner-serialized, no driver/transport I/O.
Копировать local info;1/0, owner-serialized, без driver I/O.

## Return value

`int`; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::carrier; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int bidir = phyinfo(phy, &info) && (info.capabilities & PHY_BIDIR_SESSION);
```

[Complete calling context](../../client/phy.c#L35)

## See also

- [Implementation: remote_phy/phy.c:97](../../remote_phy/phy.c#L97)
- [Public declaration](../../remote_phy/phy.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
