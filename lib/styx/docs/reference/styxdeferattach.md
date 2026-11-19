# styxdeferattach

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxdeferattach.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

StyxAttach *styxdeferattach(Styxserver *server, void *aux, void (*release)(void *));
```

## Library

Styx::server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `server` | `Styxserver *server` | Original server instance and dispatcher-owned state. |
| `aux` | `void *aux` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `void (*release)(void *)` | `void (*release)(void *)` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxdeferattach: libstyx/attach implementation and native state.

### Preserved implementation/header contract

EN/RU: Socket-owner root selection, pending NULL uses current attach;0/-1, no authority grant.
Выбор root на socket-owner; pending NULL — текущий attach;0/-1, без выдачи authority.

## Return value

`StyxAttach *`; direct result constants in this body: nil. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
StyxAttach *styxdeferattach(Styxserver *, void *aux, void (*release)(void *));
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-001.inc#L420)

## See also

- [Implementation: vendor/inferno/tools/libstyx/attach.c:95](../../vendor/inferno/tools/libstyx/attach.c#L95)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
