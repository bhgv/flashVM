# styxclientclunklater

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientclunklater.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

void styxclientclunklater(StyxFid *fid);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `fid` | `StyxFid *fid` | Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid. |

## Description

styxclientclunklater: native client clunk operations and lifetime.

### Preserved implementation/header contract

EN/RU: Consume fid without RPC wait; native worker owns client/number only.
Setup failure fails the connection, never leaks caller ownership or waits for a guest.
Поглотить fid без RPC wait; worker владеет client/number; setup failure завершает connection.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
styxclientclunklater(fid);
```

[Complete calling context](../../client/clunk.c#L63)

## See also

- [Implementation: client/clunk.c:21](../../client/clunk.c#L21)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
