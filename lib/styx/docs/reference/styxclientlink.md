# styxclientlink

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientlink.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

int styxclientlink(StyxClient *client, StyxLinkEvent *output);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `client` | `StyxClient *client` | Native client/session context; retain or consume it exactly as specified by this API. |
| `output` | `StyxLinkEvent *output` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |

## Description

styxclientlink: native client fd operations and lifetime.

### Preserved implementation/header contract

EN/RU: Copy local latest observation without I/O;1/0, output untouched on invalid input.
Копировать latest local observation без I/O;1/0, invalid не меняет output.

## Return value

`int`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int styxclientlink(StyxClient *client, StyxLinkEvent *output);
```

[Complete calling context](../../client/readable_parts/client-h/part-002.inc#L82)

## See also

- [Implementation: client/fd.c:174](../../client/fd.c#L174)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
