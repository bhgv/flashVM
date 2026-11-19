# styxclientwalktarget

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientwalktarget.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

int styxclientwalktarget(StyxClient *client, WalkEntity *root);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `client` | `StyxClient *client` | Native client/session context; retain or consume it exactly as specified by this API. |
| `root` | `WalkEntity *root` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxclientwalktarget: native client traversal operations and lifetime.

### Preserved implementation/header contract

EN/RU: Copy acknowledged fixed server projection identity,1/0; no I/O or new authority, inputs borrowed.
Скопировать acknowledged server projection identity,1/0; без I/O/new authority, inputs borrowed.

## Return value

`int`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int styxclientwalktarget(StyxClient *client, WalkEntity *root);
```

[Complete calling context](../../client/readable_parts/client-h/part-001.inc#L168)

## See also

- [Implementation: client/traversal.c:79](../../client/traversal.c#L79)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
