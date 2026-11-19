# styxclientalive

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientalive.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

int styxclientalive(StyxClient *client);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `client` | `StyxClient *client` | Native client/session context; retain or consume it exactly as specified by this API. |

## Description

styxclientalive: native client session operations and lifetime.

### Preserved implementation/header contract

EN/RU: Snapshot terminal transport/framing state under connection lock;1/0.
Not a heartbeat or a permission check. Снимок terminal state; не heartbeat/права.

## Return value

`int`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int styxclientalive(StyxClient *client);
```

[Complete calling context](../../client/readable_parts/client-h/part-001.inc#L426)

## See also

- [Implementation: client/readable_parts/session-c/part-001.inc:332](../../client/readable_parts/session-c/part-001.inc#L332)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
