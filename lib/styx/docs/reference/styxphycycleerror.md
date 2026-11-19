# styxphycycleerror

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxphycycleerror.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/server/server.h>

char *styxphycycleerror(Styxserver *server);
```

## Library

Styx::phy_server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `server` | `Styxserver *server` | Original server instance and dispatcher-owned state. |

## Description

styxphycycleerror: phy/traversal implementation and native state.

### Preserved implementation/header contract

EN/RU: Return a standard Rerror text while explicitly marking this currently executing callback's result as a cycle.
Server owner/current synchronous callback only; does not grant access, pause a guest, or guess from error text.
Use only when the actual VFS result is DVM_VFS_CYCLE. Raw peers receive ordinary Rerror; profile peers also get a typed flag.
Вернуть Rerror text и явно отметить cycle текущего synchronous callback. Только server owner; без guesses/authority.
Вызывать лишь для фактического DVM_VFS_CYCLE; raw peer получает обычный Rerror, profile также typed flag.

## Return value

`char *`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::phy_server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
reply.ename = styxphycycleerror(server);
```

[Complete calling context](../../server/phy/traversal.c#L197)

## See also

- [Implementation: server/phy/traversal.c:165](../../server/phy/traversal.c#L165)
- [Public declaration](../../server/phy/server.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
