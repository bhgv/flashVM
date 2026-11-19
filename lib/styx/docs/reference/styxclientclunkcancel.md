# styxclientclunkcancel

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientclunkcancel.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

void styxclientclunkcancel(StyxFid *fid, StyxCancel *cancel);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `fid` | `StyxFid *fid` | Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid. |
| `cancel` | `StyxCancel *cancel` | Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract. |

## Description

styxclientclunkcancel: native client clunk operations and lifetime.

### Preserved implementation/header contract

EN/RU: Consume owned fid always; cancellable Tclunk on carrier-owner, async cleanup on legacy.
An unacknowledged request schedules one independent best-effort cleanup; never reuses fid numbers.
Caller serializes/pins fid before transfer. Token is borrowed single-use, NULL keeps deadline.
Поглотить fid всегда; carrier-owner Tclunk отменяем, legacy cleanup async; без Rclunk
один independent best-effort cleanup. Token borrowed single-use, NULL оставляет deadline.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
void styxclientclunkcancel(StyxFid *fid, StyxCancel *cancel);
```

[Complete calling context](../../client/readable_parts/client-h/part-003.inc#L124)

## See also

- [Implementation: client/clunk.c:54](../../client/clunk.c#L54)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
