# styxclientclunk

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientclunk.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

void styxclientclunk(StyxFid *fid);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `fid` | `StyxFid *fid` | Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid. |

## Description

styxclientclunk: native client fid operations and lifetime.

### Preserved implementation/header contract

EN/RU: Consume one fid, clunk if transport usable, then drop its connection pin.
Поглотить fid, clunk если transport пригоден; NULL допустим.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
styxclientclunk(fid);
```

[Complete calling context](../../examples/client.c#L80)

## See also

- [Implementation: client/fid.c:21](../../client/fid.c#L21)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
