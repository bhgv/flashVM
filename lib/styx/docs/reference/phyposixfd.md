# phyposixfd

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](phyposixfd.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/phy/drivers/posix/fd.h>

RemotePhy *phyposixfd(int descriptor, unsigned capabilities);
```

## Library

Styx::carrier; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `descriptor` | `int descriptor` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `capabilities` | `unsigned capabilities` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

phyposixfd: posix/fd implementation and native state.

### Preserved implementation/header contract

EN/RU: Consume fd on every outcome; owned physical adapter/NULL, no open/dial/authentication.
Caller explicitly asserts carrier capabilities; fd alone does not prove reliable framing.
Supports O_RDWR sockets and byte devices; configures NONBLOCK/CLOEXEC, preserves other flags.
Host configures serial/bus details before adoption; wake uses a private native pipe.
Принять fd всегда; свой adapter/NULL, без open/dial/auth. Caller явно задаёт capabilities;
O_RDWR socket/byte device, NONBLOCK/CLOEXEC; serial/bus configuration делает host до adoption.

## Return value

`RemotePhy *`; direct result constants in this body: NULL. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::carrier; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), msize, user, name, timeout, limit);
```

[Complete calling context](../../client/fd.c#L37)

## See also

- [Implementation: remote_phy/drivers/posix/fd.c:317](../../remote_phy/drivers/posix/fd.c#L317)
- [Public declaration](../../remote_phy/drivers/posix/fd.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
