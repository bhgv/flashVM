# styxclientobserve

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientobserve.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

StyxWatch *styxclientobserve(StyxClient *client, StyxLinkReceive receive, void *state, void (*release)(void *));
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `client` | `StyxClient *client` | Native client/session context; retain or consume it exactly as specified by this API. |
| `receive` | `StyxLinkReceive receive` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `state` | `void *state` | Opaque callback context; its lifetime/transfer is specified by the creating API. |
| `void (*release)(void *)` | `void (*release)(void *)` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxclientobserve: native client fd operations and lifetime.

### Preserved implementation/header contract

EN/RU: Own state only on success; persistent watch/NULL, immediate current snapshot.
Caller pins client; ordered generations fence replay racing with native delivery.
Detach with clientunwatch; release follows all admitted callbacks, no guest calls.
Persistent watch/NULL принимает state при успехе; snapshot немедленно, generations ordered.

## Return value

`StyxWatch *`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
StyxWatch *styxclientobserve(StyxClient *client, StyxLinkReceive receive, void *state, void (*release)(void *));
```

[Complete calling context](../../client/readable_parts/client-h/part-002.inc#L63)

## See also

- [Implementation: client/fd.c:153](../../client/fd.c#L153)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
