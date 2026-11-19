# styxattachclient

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxattachclient.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

Client *styxattachclient(StyxAttach *request);
```

## Library

Styx::server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `request` | `StyxAttach *request` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxattachclient: libstyx/attach implementation and native state.

### Preserved implementation/header contract

EN/RU: Borrow live pending client only on its owner thread, nil otherwise.
Заимствовать live pending client только на owner-thread, иначе nil.

## Return value

`Client *`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
Client *styxattachclient(StyxAttach *); /* Borrow live pending client, nil after retirement. */
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-001.inc#L433)

## See also

- [Implementation: vendor/inferno/tools/libstyx/attach.c:128](../../vendor/inferno/tools/libstyx/attach.c#L128)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
