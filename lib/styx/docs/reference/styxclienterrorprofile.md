# styxclienterrorprofile

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclienterrorprofile.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

int styxclienterrorprofile(StyxClient *client, unsigned profile);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `client` | `StyxClient *client` | Native client/session context; retain or consume it exactly as specified by this API. |
| `profile` | `unsigned profile` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

styxclienterrorprofile: native client walk operations and lifetime.

### Preserved implementation/header contract

EN/RU: Select known error vocabulary before sharing client or allocating walk fids;1/0.
No I/O/authentication, unknown profile/late change refuses. Default is OPAQUE.
Host attests server vocabulary, not access rights; partial walks never imply absence.
Выбрать словарь ошибок до sharing/walk;1/0, без I/O/auth. Default OPAQUE;
unknown/late change refused, partial walk не означает absence.

## Return value

`int`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int styxclienterrorprofile(StyxClient *client, unsigned profile);
```

[Complete calling context](../../client/readable_parts/client-h/part-001.inc#L240)

## See also

- [Implementation: client/walk.c:330](../../client/walk.c#L330)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
