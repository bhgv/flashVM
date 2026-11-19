# styxinit

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxinit.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

char *styxinit(Styxserver *server, Styxops *ops, char *port, int mode, int needfile);
```

## Library

Styx::phy_server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `server` | `Styxserver *server` | Original server instance and dispatcher-owned state. |
| `ops` | `Styxops *ops` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `port` | `char *port` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `mode` | `int mode` | Requested operation/permission mode; use the documented protocol constants, not a pathname suffix. |
| `needfile` | `int needfile` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

Initialize an original Styx server, its root and selected platform listener.

### Preserved implementation/header contract

EN/RU: Legacy constructor cannot silently create a network listener in the PHY backend.
Legacy constructor не создаёт network listener в PHY backend.

## Return value

`char *`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::phy_server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
char *error = styxinit(&server, &ops, argc > 1 ? argv[1] : "0", 0555, 0); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
```

[Complete calling context](../../examples/server.c#L364)

## See also

- [Implementation: server/phy/lifetime.c:111](../../server/phy/lifetime.c#L111)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
