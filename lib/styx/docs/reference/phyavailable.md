# phyavailable

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](phyavailable.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/phy/phy.h>

int phyavailable(RemotePhy *phy, size_t *bytes);
```

## Library

Styx::carrier; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `phy` | `RemotePhy *phy` | Physical-carrier capability; ownership transfer depends on the constructor/operation. |
| `bytes` | `size_t *bytes` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |

## Description

phyavailable: remote_phy/phy implementation and native state.

### Preserved implementation/header contract

EN/RU: Optional local byte hint, not EOF or permission; owner-serialized, output unchanged on failure.
Optional byte hint, не EOF/permission; owner-serialized, output сохраняется при failure.

## Return value

`int`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::carrier; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
good = good && phyavailable(right, &hint) == PHY_UNSUPPORTED && hint == 123;
```

[Complete calling context](../../tests/memory_contract.c#L140)

## See also

- [Implementation: remote_phy/phy.c:278](../../remote_phy/phy.c#L278)
- [Public declaration](../../remote_phy/phy.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
