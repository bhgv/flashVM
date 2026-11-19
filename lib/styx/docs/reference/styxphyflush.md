# styxphyflush

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxphyflush.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/server/server.h>

int styxphyflush(Styxserver *server, unsigned timeout);
```

## Library

Styx::phy_server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `server` | `Styxserver *server` | Original server instance and dispatcher-owned state. |
| `timeout` | `unsigned timeout` | Native wait budget in milliseconds; zero and upper-bound behavior follow the function contract. |

## Description

styxphyflush: phy/io implementation and native state.

### Preserved implementation/header contract

EN/RU: Owner flushes staged output within one monotonic budget0..120000ms;1 drained/0 pending/failure.
Partial known prefix is retained, uncertain output terminally fails; no automatic replay/reconnect.
Native wait only, not guest cooperation. Explicit styxend discards any remaining output.
Owner flush с единым budget0..120000ms;1 drained/0 pending/failure, tail сохраняется,
uncertain terminal, без replay/reconnect. styxend явно отбрасывает остаток.

## Return value

`int`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::phy_server; hosted native build

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: server/phy/io.c:246](../../server/phy/io.c#L246)
- [Public declaration](../../server/phy/server.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
