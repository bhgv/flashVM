# styxclientfree

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientfree.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

void styxclientfree(StyxClient *client);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `client` | `StyxClient *client` | Native client/session context; retain or consume it exactly as specified by this API. |

## Description

styxclientfree: native client session operations and lifetime.

### Preserved implementation/header contract

EN/RU: Drop connection reference after callers retire; live fids pin transport.
Final release closes transport once; server retires root on disconnect. NULL allowed.
Поглотить ссылку; live fids удерживают transport; NULL допустим.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
styxclientfree(client);
```

[Complete calling context](../../examples/client.c#L86)

## See also

- [Implementation: client/readable_parts/session-c/part-001.inc:358](../../client/readable_parts/session-c/part-001.inc#L358)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
