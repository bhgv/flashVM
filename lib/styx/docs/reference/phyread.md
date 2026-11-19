# phyread

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](phyread.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/phy/phy.h>

PhyTransfer phyread(RemotePhy *phy, void *bytes, size_t size);
```

## Library

Styx::carrier; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `phy` | `RemotePhy *phy` | Physical-carrier capability; ownership transfer depends on the constructor/operation. |
| `bytes` | `void *bytes` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |

## Description

phyread: remote_phy/phy implementation and native state.

### Preserved implementation/header contract

EN/RU: Owner-serialized nonblocking read/write, borrowed buffers; validate precise driver results.
Size0 succeeds without a callback; cap hints bound a single driver request.
Owner-serialized read/write, buffers borrowed; size0 без callback, cap limits учитываются.

## Return value

`PhyTransfer`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::carrier; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
PhyTransfer out = phyread(mux->phy, mux->receive + mux->used, mux->want - mux->used);
```

[Complete calling context](../../client/mux_loop.c#L317)

## See also

- [Implementation: remote_phy/phy.c:155](../../remote_phy/phy.c#L155)
- [Public declaration](../../remote_phy/phy.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
