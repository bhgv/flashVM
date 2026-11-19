# styxclientshutdown

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientshutdown.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

void styxclientshutdown(StyxClient *client);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `client` | `StyxClient *client` | Native client/session context; retain or consume it exactly as specified by this API. |

## Description

styxclientshutdown: native client fd operations and lifetime.

### Preserved implementation/header contract

EN/RU: Explicit shutdown on native I/O worker, may wait for native owner retirement.
No guest execution/pause; final clientfree itself remains nonwaiting.
Явный native shutdown может ждать owner retirement; clientfree не ждёт.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
styxclientshutdown(client); /* EN: Invoke styxclientshutdown with these explicit arguments. RU: Вызвать styxclientshutdown с этими явными аргументами. */
```

[Complete calling context](../../examples/client.c#L85)

## See also

- [Implementation: client/fd.c:84](../../client/fd.c#L84)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
