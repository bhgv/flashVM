# phywait

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](phywait.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/phy/phy.h>

int phywait(RemotePhy *phy, unsigned wanted, int timeout, unsigned *ready);
```

## Library

Styx::carrier; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `phy` | `RemotePhy *phy` | Physical-carrier capability; ownership transfer depends on the constructor/operation. |
| `wanted` | `unsigned wanted` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `timeout` | `int timeout` | Native wait budget in milliseconds; zero and upper-bound behavior follow the function contract. |
| `ready` | `unsigned *ready` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

phywait: remote_phy/phy implementation and native state.

### Preserved implementation/header contract

EN/RU: Owner waits for requested READ/WRITE; timeout>=-1,0 polls, returns OK/ERROR and ready bits.
No OS descriptor in this contract; driver integrates its own bus/IRQ/event mechanism.
Owner ждёт READ/WRITE, timeout>=-1;0 poll; OK/ERROR и ready bits, без требования OS fd.

## Return value

`int`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::carrier; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int good = phywait(test->phy, PHY_READ, -1, &ready) == PHY_OK && (ready & PHY_WAKE);
```

[Complete calling context](../../tests/memory_contract.c#L43)

## See also

- [Implementation: remote_phy/phy.c:227](../../remote_phy/phy.c#L227)
- [Public declaration](../../remote_phy/phy.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
