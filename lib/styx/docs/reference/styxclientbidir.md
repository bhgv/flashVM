# styxclientbidir

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientbidir.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

int styxclientbidir(StyxClient *client);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `client` | `StyxClient *client` | Native client/session context; retain or consume it exactly as specified by this API. |

## Description

styxclientbidir: native client phy operations and lifetime.

### Preserved implementation/header contract

EN/RU: Borrow live client;1 explicitly bidir-session carrier/0 ordinary or NULL.
This immutable local capability is neither reverse-server liveness nor access authority.
Borrow live client;1 bidir-session carrier/0 ordinary/NULL, не liveness/authority.

## Return value

`int`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int styxclientbidir(StyxClient *client);
```

[Complete calling context](../../client/readable_parts/client-h/part-001.inc#L220)

## See also

- [Implementation: client/phy.c:59](../../client/phy.c#L59)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
