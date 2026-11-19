# styxattachreply

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxattachreply.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

int styxattachreply(StyxAttach *request, char *error);
```

## Library

Styx::server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `request` | `StyxAttach *request` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `error` | `char *error` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxattachreply: libstyx/attach implementation and native state.

### Preserved implementation/header contract

Borrow live pending client, nil after retirement.

## Return value

`int`; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int styxattachreply(StyxAttach *, char *error);
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-002.inc#L18)

## See also

- [Implementation: vendor/inferno/tools/libstyx/attach.c:196](../../vendor/inferno/tools/libstyx/attach.c#L196)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
